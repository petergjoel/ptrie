/* 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 *  Copyright Morten K. Schou
 */

/* 
 * File:   ptrie_interface.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 18-12-2020.
 */

#ifndef PTRIE_INTERFACE_H
#define PTRIE_INTERFACE_H

#include <ptrie/ptrie_map.h>
#include <boost/mp11.hpp>
#include <vector>
#include <variant>

namespace ptrie {
    // This file defines interfacing functionality for using more types with ptrie.
    // ptrie_interface provides a unified interface for already supported types, std::vector of such types, std::string,
    // and types that get support from its specialization of ptrie_interface by conversion to/from std::vector<std::byte>.
    //
    // This file provides specializations for KEY where:
    //  a) std::has_unique_object_representations_v<KEY> is true or ptrie::byte_iterator<KEY> is explicitly defined
    //  b) Nested combinations of std::tuple and std::vector, but without vectors nested (somewhere) inside other vectors,
    //  where the lowest element types satisfies a). The size of std::vector is stored to make parsing consistent.
    //
    // Lastly, for convenience, the file provides ptrie_set and ptrie_map that uses ptrie_interface directly.

    template <typename KEY, typename = void> struct ptrie_interface;
    template <typename KEY, typename = void> struct has_ptrie_interface : std::false_type {};
    template <typename KEY> struct has_ptrie_interface<KEY, std::void_t<ptrie_interface<KEY>>> : std::true_type {};
    template <typename KEY> constexpr bool has_ptrie_interface_v = has_ptrie_interface<KEY>::value;

    template <typename KEY, typename = void> struct has_custom_byte_iterator : std::false_type {}; // Can be specialized by user types.
    template <typename KEY> constexpr bool has_byte_iterator_v = std::has_unique_object_representations_v<KEY> || has_custom_byte_iterator<KEY>::value;

    // First define instances for resp. value and vector of values that trivially fits into a ptrie.
    template <typename KEY>
    struct ptrie_interface<KEY, std::enable_if_t<has_byte_iterator_v<KEY>>> {
        using elem_type = KEY;
        using insert_type = KEY;
        using external_type = KEY;
        static constexpr insert_type to_ptrie(const external_type& key) {
            return key;
        }
        template<typename PT>
        static constexpr external_type unpack(const PT& p, size_t id) {
            static_assert(std::is_same_v<size_t , decltype(std::declval<PT>().unpack(std::declval<size_t>(),std::declval<elem_type*>()))>);
            external_type res;
            p.unpack(id, &res);
            return res;
        }
    };
    template <typename KEY>
    struct ptrie_interface<std::vector<KEY>, std::enable_if_t<has_byte_iterator_v<KEY>>> {
        using elem_type = KEY;
        using insert_type = std::vector<KEY>;
        using external_type = std::vector<KEY>;
        static insert_type to_ptrie(const external_type& key) {
            return std::move(key);
        }
        template<typename PT>
        static external_type unpack(const PT& p, size_t id) {
            static_assert(std::is_same_v<std::vector<elem_type>, decltype(std::declval<PT>().unpack(std::declval<size_t>()))>);
            return p.unpack(id);
        }
    };
    template<class CharT>
    struct ptrie_interface<std::basic_string<CharT>> {
        using elem_type = CharT;
        using insert_type = std::pair<const CharT*, size_t>;
        using external_type = std::basic_string<CharT>;
        static insert_type to_ptrie(const external_type& key) {
            return std::make_pair(key.data(), key.length());
        }
        template<typename PT>
        static external_type unpack(const PT& p, size_t id) {
            static_assert(std::is_same_v<std::vector<elem_type>, decltype(std::declval<PT>().unpack(std::declval<size_t>()))>);
            auto vector = p.unpack(id);
            return external_type(vector.data(), vector.size());
        }
    };

    // For vectors, we need to know if elements are fixed size
    template <typename KEY, typename = void> struct fixed_byte_size;
    template <typename KEY, typename = void> struct has_fixed_byte_size : std::false_type {};
    template <typename KEY> struct has_fixed_byte_size<KEY, std::void_t<fixed_byte_size<KEY>>> : std::true_type {};
    template <typename KEY> constexpr bool has_fixed_byte_size_v = has_fixed_byte_size<KEY>::value;

    template <typename KEY, typename = void> struct byte_vector_converter;
    template <typename KEY, typename = void> struct has_byte_vector_converter : std::false_type {};
    template <typename KEY> struct has_byte_vector_converter<KEY, std::void_t<byte_vector_converter<KEY>>> : std::true_type {};
    template <typename KEY> constexpr bool has_byte_vector_converter_v = has_byte_vector_converter<KEY>::value;
    // Convenience definition
    template <typename KEY> constexpr bool has_fixed_size_converter = has_fixed_byte_size_v<KEY> && has_byte_vector_converter_v<KEY>;

    // If KEY satisfies has_byte_iterator_v<KEY> it has a fixed size.
    template <typename KEY>
    struct fixed_byte_size<KEY, std::enable_if_t<has_byte_iterator_v<KEY>>> {
        using T = KEY;
        static constexpr size_t size() {
            return ptrie::byte_iterator<KEY>::element_size();
        }
    };
    // A tuple of fixed-size Args has a fixed size.
    template <typename... Args>
    struct fixed_byte_size<std::tuple<Args...>, std::enable_if_t<((has_fixed_byte_size_v<Args>) && ...)>> {
        using T = std::tuple<Args...>;
        static constexpr size_t size() {
            return (fixed_byte_size<Args>::size() + ...);
        }
    };

    // Byte converter by using byte_iterator<KEY> directly.
    template <typename KEY>
    struct byte_vector_converter<KEY, std::enable_if_t<has_byte_iterator_v<KEY>>> {
        using T = KEY;
        static constexpr size_t size(const T&) {
            return ptrie::byte_iterator<KEY>::element_size();
        }
        static constexpr void push_back_bytes(std::vector<std::byte>& result, const T& data){
            for (size_t i = 0; i < ptrie::byte_iterator<KEY>::element_size(); ++i){
                result.push_back((std::byte)ptrie::byte_iterator<KEY>::const_access(&data, i));
            }
        }
        static constexpr void from_bytes(const std::vector<std::byte>& bytes, size_t& bytes_id, T& data){
            for (size_t i = 0; i < ptrie::byte_iterator<KEY>::element_size(); ++i, ++bytes_id){
                ptrie::byte_iterator<KEY>::access(&data, i) = (unsigned char)bytes[bytes_id];
            }
        }
    };
    // Byte converter for tuples of elements that has a byte converter that knows its size (i.e. vector stores size, so it can parse it back correctly).
    template <typename... Args>
    struct byte_vector_converter<std::tuple<Args...>, std::enable_if_t<((has_byte_vector_converter_v<Args>) && ...)>> {
        using T = std::tuple<Args...>;
        static constexpr size_t size(const T& data) {
            return std::apply([](auto&&... args){return (byte_vector_converter<std::remove_cvref_t<decltype(args)>>::size(args) + ...);}, data);
        }
        static constexpr void push_back_bytes(std::vector<std::byte>& result, const T& data){
            std::apply([&result](auto&&... args){(byte_vector_converter<std::remove_cvref_t<decltype(args)>>::push_back_bytes(result, args), ...);}, data);
        }
        static constexpr void from_bytes(const std::vector<std::byte>& bytes, size_t& bytes_id, T& data){
            std::apply([&bytes, &bytes_id](auto&&... args){(byte_vector_converter<std::remove_cvref_t<decltype(args)>>::from_bytes(bytes, bytes_id, args), ...);}, data);
        }
    };
    // Vector of fixed size elements. Is not itself fixed_size. It stores extra info of its size, so it only uses the bytes corresponding to it.
    // This means multiple vectors can be in a tuple using this approach.
    template <typename Elem>
    struct byte_vector_converter<std::vector<Elem>, std::enable_if_t<has_fixed_size_converter<Elem> && has_fixed_size_converter<typename std::vector<Elem>::size_type>>> {
        using T = std::vector<Elem>;
        using size_type = typename std::vector<Elem>::size_type;
        static constexpr size_t size(const T& data) {
            return fixed_byte_size<size_type>::size() + fixed_byte_size<Elem>::size() * data.size();
        }
        static constexpr void push_back_bytes(std::vector<std::byte>& result, const T& data){
            byte_vector_converter<size_type>::push_back_bytes(result, data.size());
            for (const auto& elem : data) {
                byte_vector_converter<Elem>::push_back_bytes(result, elem);
            }
        }
        static constexpr void from_bytes(const std::vector<std::byte>& bytes, size_t& bytes_id, T& data){
            size_type size;
            byte_vector_converter<size_type>::from_bytes(bytes, bytes_id, size);
            for (size_type i = 0; i < size; ++i) {
                data.emplace_back();
                byte_vector_converter<Elem>::from_bytes(bytes, bytes_id, data.back());
            }
        }
    };

    // Support strings in tuples and vectors.
    template<class CharT>
    struct byte_vector_converter<std::basic_string<CharT>, std::enable_if_t<has_fixed_size_converter<CharT>>> {
        using T = std::basic_string<CharT>;
        using size_type = typename T::size_type;
        static size_t size(const T& data) {
            return fixed_byte_size<size_type>::size() + fixed_byte_size<CharT>::size() * data.size();
        }
        static void push_back_bytes(std::vector<std::byte>& result, const T& data){
            byte_vector_converter<size_type>::push_back_bytes(result, data.size());
            for (const auto& elem : data) {
                byte_vector_converter<CharT>::push_back_bytes(result, elem);
            }
        }
        static void from_bytes(const std::vector<std::byte>& bytes, size_t& bytes_id, T& data){
            size_type size;
            byte_vector_converter<size_type>::from_bytes(bytes, bytes_id, size);
            data = T(reinterpret_cast<const CharT*>(&bytes[bytes_id]), size);
            bytes_id += fixed_byte_size<CharT>::size() * size;
        }
    };

    // Byte converter for variants of elements that has a byte converter that knows its size (i.e. vector stores size, so it can parse it back correctly).
    template <typename... Args>
    struct byte_vector_converter<std::variant<Args...>, std::enable_if_t<((has_byte_vector_converter_v<Args>) && ...)>> {
        static constexpr size_t variant_size = std::variant_size_v<std::variant<Args...>>;
        using T = std::variant<Args...>;
        using index_type = std::conditional_t<(variant_size <= std::numeric_limits<unsigned char>::max()), unsigned char, size_t>; // Just use one byte if variant has less than 256 options, which is common.
        static constexpr size_t size(const T& data) {
            if constexpr (variant_size == 1) {
                return byte_vector_converter<std::variant_alternative_t<0,T>>::size(std::get<0>(data));
            } else {
                return fixed_byte_size<index_type>::size() +
                       boost::mp11::mp_with_index<variant_size>(data.index(), [&data](auto I) {
                           return byte_vector_converter<std::variant_alternative_t<I, T>>::size(std::get<I>(data));
                       });
            }
        }
        static constexpr void push_back_bytes(std::vector<std::byte>& result, const T& data){
            if constexpr (variant_size == 1) {
                byte_vector_converter<std::variant_alternative_t<0,T>>::push_back_bytes(result, std::get<0>(data));
            } else {
                if (data.valueless_by_exception()) { // This would mess up the static_cast, but should not happen in general.
                    assert(false); return;
                }
                byte_vector_converter<index_type>::push_back_bytes(result, static_cast<index_type>(data.index()));
                boost::mp11::mp_with_index<variant_size>(data.index(), [&result, &data](auto I) {
                    byte_vector_converter<std::variant_alternative_t<I, T>>::push_back_bytes(result, std::get<I>(data));
                });
            }
        }
        static constexpr void from_bytes(const std::vector<std::byte>& bytes, size_t& bytes_id, T& data){
            if constexpr (variant_size == 1) {
                byte_vector_converter<std::variant_alternative_t<0,T>>::from_bytes(bytes, bytes_id, std::get<0>(data));
            } else {
                index_type index = 0;
                byte_vector_converter<index_type>::from_bytes(bytes, bytes_id, index);
                boost::mp11::mp_with_index<variant_size>(index, [&bytes,&bytes_id,&data](auto I){
                    byte_vector_converter<std::variant_alternative_t<I,T>>::from_bytes(bytes, bytes_id, data.template emplace<I>());
                });
            }
        }
    };

    template <typename KEY, typename = void> struct is_vector_of_byte_iterator : std::false_type{};
    template <typename KEY> struct is_vector_of_byte_iterator<std::vector<KEY>, std::enable_if_t<has_byte_iterator_v<KEY>>> : std::true_type{};
    template <typename KEY> constexpr bool is_vector_of_byte_iterator_v = is_vector_of_byte_iterator<KEY>::value;
    template <typename KEY, typename = void> struct is_basic_string : std::false_type{};
    template <class CharT> struct is_basic_string<std::basic_string<CharT>> : std::true_type{};
    template <typename KEY> constexpr bool is_basic_string_v = is_basic_string<KEY>::value;

    // Special ptrie_interface for any KEY with a byte vector converter.
    template <typename KEY>
    struct ptrie_interface<KEY, std::enable_if_t<has_byte_vector_converter_v<KEY> && !has_byte_iterator_v<KEY> && !is_vector_of_byte_iterator_v<KEY> && !is_basic_string_v<KEY>>> {
        using elem_type = std::byte;
        using insert_type = std::vector<std::byte>;
        using external_type = KEY;
        static insert_type to_ptrie(const external_type& key) {
            std::vector<std::byte> result;
            result.reserve(byte_vector_converter<KEY>::size(key));
            byte_vector_converter<KEY>::push_back_bytes(result, key);
            return result;
        }
        template<typename PT>
        static external_type unpack(const PT& p, size_t id) {
            static_assert(std::is_same_v<std::vector<elem_type>, decltype(std::declval<PT>().unpack(std::declval<size_t>()))>);
            auto bytes = p.unpack(id);
            external_type result;
            size_t bytes_id = 0;
            byte_vector_converter<KEY>::from_bytes(bytes, bytes_id, result);
            return result;
        }
    };

    template <typename KEY>
    using ptrie_interface_elem = typename ptrie_interface<KEY>::elem_type;

    // Next we define ptrie_set and ptrie_map which makes using ptrie_interface seamless (except now you should use 'at' instead of 'unpack').
    template <typename KEY>
    class ptrie_set : private ptrie::set_stable<ptrie_interface_elem<KEY>> {
        static_assert(has_ptrie_interface_v<KEY>, "KEY does not provide a specialization for ptrie_interface.");
        static_assert(std::is_same_v<KEY, typename ptrie_interface<KEY>::external_type>, "KEY not matching ptrie_interface<KEY>::external_type.");
        using pt = ptrie::set_stable<ptrie_interface_elem<KEY>>;
    public:
        using elem_type = KEY;

        using typename pt::set_stable;
        using pt::unpack;
        using pt::size;

        std::pair<bool, size_t> insert(const KEY& key) {
            return pt::insert(ptrie_interface<KEY>::to_ptrie(key));
        }
        [[nodiscard]] std::pair<bool, size_t> exists(const KEY& key) const {
            return pt::exists(ptrie_interface<KEY>::to_ptrie(key));
        }
        bool erase (const KEY& key) {
            return pt::erase(ptrie_interface<KEY>::to_ptrie(key));
        }
        KEY at(size_t index) const {
            return ptrie_interface<KEY>::unpack(*this, index);
        }
    };

    template <typename KEY,typename T>
    class ptrie_map : private ptrie::map<ptrie_interface_elem<KEY>, T> {
        static_assert(has_ptrie_interface_v<KEY>, "KEY does not provide a specialization for ptrie_interface.");
        static_assert(std::is_same_v<KEY, typename ptrie_interface<KEY>::external_type>, "KEY not matching ptrie_interface<KEY>::external_type.");
        using pt = ptrie::map<ptrie_interface_elem<KEY>, T>;
    public:
        using elem_type = KEY;

        using typename pt::map;
        using pt::unpack;
        using pt::size;
        using pt::get_data;

        // Yes, insert(), exists(), erase() and at() are same as ptrie_set, but I tried multiple inheritance, and it didn't fly.
        std::pair<bool, size_t> insert(const KEY& key) {
            return pt::insert(ptrie_interface<KEY>::to_ptrie(key));
        }
        [[nodiscard]] std::pair<bool, size_t> exists(const KEY& key) const {
            return pt::exists(ptrie_interface<KEY>::to_ptrie(key));
        }
        bool erase (const KEY& key) {
            return pt::erase(ptrie_interface<KEY>::to_ptrie(key));
        }
        KEY at(size_t index) const {
            return ptrie_interface<KEY>::unpack(*this, index);
        }
        T& operator[](const KEY& key) {
            return pt::operator[](ptrie_interface<KEY>::to_ptrie(key));
        }
    };
}

#endif //PTRIE_INTERFACE_H
