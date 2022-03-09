/* 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 *  Copyright Morten K. Schou
 */

/* 
 * File:   interface
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 09-03-2022.
 */

#define BOOST_TEST_MODULE interface

#include <boost/test/unit_test.hpp>
#include <ptrie/ptrie_interface.h>

using namespace ptrie;

struct my_struct {
    size_t a;
    size_t b;
    my_struct() = default;
    my_struct(size_t a, size_t b) : a(a), b(b) {};
    friend bool operator==(const my_struct& lhs, const my_struct& rhs) { return lhs.a == rhs.a && lhs.b == rhs.b; };
    friend bool operator!=(const my_struct& lhs, const my_struct& rhs) { return !(lhs==rhs); };
    friend std::ostream& operator<<(std::ostream& s, const my_struct& m) {
        return s << "{a: " << m.a << ", b: " << m.b << "}";
    }
};

BOOST_AUTO_TEST_CASE(ptrie_interface_string_Test)
{
    ptrie_set<std::string> string_set;
    auto [fresh1, id1] = string_set.insert("A string");
    BOOST_CHECK(fresh1);
    BOOST_CHECK_EQUAL(id1, 0);
    std::string another_string = "Another string";
    auto [fresh2, id2] = string_set.insert(another_string);
    BOOST_CHECK(fresh2);
    BOOST_CHECK_EQUAL(id2, 1);

    std::string same_string = "Another string";
    auto [fresh3, id3] = string_set.insert(same_string);
    BOOST_CHECK(!fresh3);
    BOOST_CHECK_EQUAL(id3, 1);

    auto first_string = string_set.at(0);
    BOOST_CHECK_EQUAL(first_string, "A string");

    auto second_string = string_set.at(1);
    BOOST_CHECK_EQUAL(second_string, another_string);
}

BOOST_AUTO_TEST_CASE(ptrie_interface_tuple_Test)
{
    ptrie_set<std::tuple<int,std::u16string,my_struct>> string_set;
    my_struct m{42,100};
    auto [fresh1, id1] = string_set.insert(std::make_tuple(1, u"a string", m));
    BOOST_CHECK(fresh1);
    BOOST_CHECK_EQUAL(id1, 0);

    auto [i1, s1, m1] = string_set.at(0);
    BOOST_CHECK_EQUAL(i1, 1);
    BOOST_CHECK(s1 == u"a string");
    BOOST_CHECK_EQUAL(m1, m);
}

BOOST_AUTO_TEST_CASE(ptrie_interface_vector_Test)
{
    ptrie_set<std::vector<my_struct>> string_set;
    std::vector<my_struct> my_vector;
    my_vector.emplace_back(42, 100);
    my_vector.emplace_back(9000, 2);
    auto [fresh1, id1] = string_set.insert(my_vector);
    BOOST_CHECK(fresh1);
    BOOST_CHECK_EQUAL(id1, 0);

    auto v = string_set.at(0);
    BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(), my_vector.begin(), my_vector.end());
}

BOOST_AUTO_TEST_CASE(ptrie_interface_variant_Test)
{
    ptrie_set<std::variant<int,std::u16string,my_struct>> string_set;
    auto [fresh1, id1] = string_set.insert(1);
    BOOST_CHECK(fresh1);
    BOOST_CHECK_EQUAL(id1, 0);
    auto [fresh2, id2] = string_set.insert(u"a string");
    BOOST_CHECK(fresh2);
    BOOST_CHECK_EQUAL(id2, 1);
    auto [fresh3, id3] = string_set.insert(my_struct(42,100));
    BOOST_CHECK(fresh3);
    BOOST_CHECK_EQUAL(id3, 2);

    auto v1 = string_set.at(0);
    BOOST_CHECK_EQUAL(v1.index(), 0);
    BOOST_CHECK_EQUAL(std::get<0>(v1), 1);
    auto v2 = string_set.at(1);
    BOOST_CHECK_EQUAL(v2.index(), 1);
    BOOST_CHECK(std::get<1>(v2) == u"a string");
    auto v3 = string_set.at(2);
    BOOST_CHECK_EQUAL(v3.index(), 2);
    BOOST_CHECK_EQUAL(std::get<2>(v3), my_struct(42,100));
}

BOOST_AUTO_TEST_CASE(ptrie_interface_tuple_of_vector_Test)
{
    ptrie_set<std::tuple<std::vector<my_struct>,std::string,std::vector<int>>> string_set;
    std::vector<my_struct> v1;
    v1.emplace_back(42,100);
    v1.emplace_back(9000, 2);
    v1.emplace_back(4, 10);
    v1.emplace_back(29000,72);
    v1.emplace_back(9000, 2);
    std::vector<int> v2;
    v2.emplace_back(-5);
    v2.emplace_back(123456789);
    auto [fresh1, id1] = string_set.insert(std::make_tuple(v1, "a string", v2));
    BOOST_CHECK(fresh1);
    BOOST_CHECK_EQUAL(id1, 0);

    auto [ov1, s, ov2] = string_set.at(0);
    BOOST_CHECK_EQUAL_COLLECTIONS(ov1.begin(), ov1.end(), v1.begin(), v1.end());
    BOOST_CHECK_EQUAL(s, "a string");
    BOOST_CHECK_EQUAL_COLLECTIONS(ov2.begin(), ov2.end(), v2.begin(), v2.end());
}

BOOST_AUTO_TEST_CASE(ptrie_interface_map_string_Test)
{
    ptrie_map<std::string,int> string_map;
    auto [fresh1, id1] = string_map.insert("A string");
    BOOST_CHECK(fresh1);
    BOOST_CHECK_EQUAL(id1, 0);
    string_map.get_data(id1) = 42;

    string_map["Another string"] = 9000;
    auto [exists, id2] = string_map.exists("Another string");
    BOOST_CHECK(exists);
    BOOST_CHECK_EQUAL(id2, 1);
    BOOST_CHECK_EQUAL(string_map.get_data(id2), 9000);

    BOOST_CHECK_EQUAL(string_map["A string"], 42);
}
