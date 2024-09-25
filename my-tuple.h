#pragma once
#include <array>
#include <cstddef>
#include <utility>
#include <functional>

namespace impl {
template<size_t ind, typename... Types>
struct IthType;

template<typename First, typename... Other>
struct IthType<0, First, Other...> {
    using type = First;
};

template<size_t ind, typename First, typename... Other>
struct IthType<ind, First, Other...> {
    using type = typename IthType<ind - 1, Other...>::type;
};


struct MemberInfo {
    size_t orig_ind;
    size_t inside_ind;
    size_t offset;
    size_t align;
    size_t sz;
};

template<typename... Types>
constexpr std::array<MemberInfo, sizeof...(Types)> GetMembersInfo() {
    std::array<MemberInfo, sizeof...(Types)> ans;

    [&ans] <size_t... OrigInds> (std::index_sequence<OrigInds...>) {
        ([&ans] <size_t orig_ind> (std::index_sequence<orig_ind>) {
            ans[orig_ind] = {
                .orig_ind = orig_ind,
                .inside_ind = orig_ind,
                .offset = 0,
                .align = alignof(typename IthType<orig_ind, Types...>::type),
                .sz = sizeof(typename IthType<orig_ind, Types...>::type)
            };
        } (std::index_sequence<OrigInds>()), ...);
    } (std::make_index_sequence<sizeof...(Types)>());


    std::sort(ans.begin(), ans.end(), [] (const MemberInfo &lhs, const MemberInfo &rhs) {
        return lhs.align > rhs.align;
    });

    size_t cur_ind = 0;
    size_t cur_offset = 0;
    for (auto &i : ans) {
        i.inside_ind = cur_ind++;
        i.offset = cur_offset;
        cur_offset += i.sz;
    }

    return ans;
}
} // namespace impl

template<typename... Types>
class Tuple {
 public:
    Tuple() {
        [&] <size_t... OrigInds> (std::index_sequence<OrigInds...>) {
            ([&] <size_t orig_ind> (std::index_sequence<orig_ind>) {
                new (storage_ + mem_infos_[orig_ind].offset) typename impl::IthType<mem_infos_[orig_ind].orig_ind, Types...>();
            }(std::index_sequence<OrigInds>()), ...);
        } (std::make_index_sequence<sizeof...(Types)>());
    }

    template<size_t Ind> requires(Ind < sizeof...(Types))
    decltype(auto) get() {
        return *reinterpret_cast<typename impl::IthType<Ind, Types...>::type*>(storage_ + mem_infos_[Ind].offset);
    }

    template<size_t Ind> requires(Ind < sizeof...(Types))
    decltype(auto) get() const {
        return *reinterpret_cast<typename impl::IthType<Ind, Types...>::type*>(storage_ + mem_infos_[Ind].offset);
    }

 private:
    static constexpr std::array<impl::MemberInfo, sizeof...(Types)> mem_infos_ = impl::GetMembersInfo<Types...>();
    alignas(Types...) std::byte storage_[mem_infos_.back().offset + mem_infos_.back().sz];
};

template<typename... Types>
struct std::tuple_size<Tuple<Types...>> : std::integral_constant<std::size_t, sizeof...(Types)> {};


template<std::size_t I, typename... Types>
struct std::tuple_element<I, Tuple<Types...>> {
    using type = typename impl::IthType<I, Types...>::type;
};

template<typename... TTypes, typename... UTypes>
constexpr std::common_comparison_category_t<TTypes..., UTypes...>
operator<=>(const Tuple<TTypes...> &lhs, const Tuple<UTypes...> &rhs) {
    int ans = 0;
    [&] <size_t... OrigInds> (std::index_sequence<OrigInds...>) {
        ([&] <size_t orig_ind> (std::index_sequence<orig_ind>) {
            if (ans == 0) {
                ans = (lhs.template get<orig_ind>() <=> rhs.template get<orig_ind>());
            }
        }(std::index_sequence<OrigInds>()), ...);
    } (std::make_index_sequence<sizeof...(TTypes)>());
    return ans;
}
