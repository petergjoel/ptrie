/*
 * Copyright Peter G. Jensen <root@petergjoel.dk>
 *  
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
// Created by Peter G. Jensen on 12/9/16.
#define BOOST_TEST_MODULE PTrieStableSet
#include <boost/test/unit_test.hpp>
#include <ptrie/ptrie_stable.h>
#include <vector>
#include "utils.h"

using namespace ptrie;
using namespace std;


BOOST_AUTO_TEST_CASE(SimpleRIterator)
{
    std::cerr << "SimpleRIterator" << std::endl;
    set_stable<size_t> set;
    constexpr size_t x = 10000;
    for(size_t i = 0; i < x; ++i)
    {
        set.insert(i);
    }
    size_t cnt = 0;
    for(auto b = --set.end(); b != set.begin(); --b)
    {
        ++cnt;
        BOOST_REQUIRE(0 <= b.index());
        BOOST_REQUIRE(b.index() <= x);
        BOOST_REQUIRE(b.index() == b.unpack().back());
    }
    BOOST_CHECK_EQUAL(cnt, x-1);
}
 

BOOST_AUTO_TEST_CASE(SimpleIterator)
{
    std::cerr << "SimpleIterator" << std::endl;
    const size_t x = 10000;
    set_stable<size_t> set;
    for(size_t i = 0; i < x; ++i)
    {
        set.insert(i);
    }
    size_t cnt = 0;
    for(auto b = set.begin(); b != set.end(); ++b)
    {
        ++cnt;
        BOOST_REQUIRE(0 <= b.index());
        BOOST_REQUIRE(b.index() <= x);
        BOOST_REQUIRE(b.index() == b.unpack().back());
    }
    BOOST_CHECK_EQUAL(cnt, x);
}

BOOST_AUTO_TEST_CASE(PseudoRand1)
{
    std::cerr << "PseudoRand1" << std::endl;
    for(size_t seed = 1337; seed < (1337+10); ++seed) {
        set_stable<> set;
        vector<size_t> ids;
        auto scratchpad = std::make_unique<unsigned char[]>(20+sizeof(size_t));
        for(size_t i = 0; i < 1024*10; ++i) {
            auto data = rand_data(i + seed, 20);
            auto res = set.insert(data.first.get(), data.second);
            BOOST_CHECK(res.first);
            ids.push_back(res.second);
            auto size = set.unpack(res.second, scratchpad.get());

            BOOST_CHECK_EQUAL(data.second, size);
            BOOST_CHECK_EQUAL(memcmp(data.first.get(), scratchpad.get(), size), 0);
        }

        // let us unwrap everything and check that it is there!
        for(size_t i = 0; i < 1024*10; ++i) {
            auto data = rand_data(i + seed, 20);
            auto size = set.unpack(ids[i], scratchpad.get());

            BOOST_CHECK_EQUAL(data.second, size);
            BOOST_CHECK_EQUAL(memcmp(data.first.get(), scratchpad.get(), size), 0);
            
            auto key = set.unpack(ids[i]);
            BOOST_CHECK_EQUAL(data.second, key.size());
            BOOST_CHECK_EQUAL(memcmp(data.first.get(), key.data(), size), 0);
        }
    }
}

BOOST_AUTO_TEST_CASE(PseudoRandSplitHeap)
{
    std::cerr << "PseudoRandSplitHeap" << std::endl;
    for(size_t seed = 42; seed < (42+10); ++seed) {
        set_stable<unsigned char, size_t, sizeof(size_t)+1, 6> set;
        vector<size_t> ids;
        auto scratchpad = std::make_unique<unsigned char[]>(20+sizeof(size_t));

        for(size_t i = 0; i < 1024*10; ++i) {
            auto data = rand_data(i + seed, 20);
            auto res = set.insert(data.first.get(), data.second);
            BOOST_CHECK(res.first);
            ids.push_back(res.second);
            auto size = set.unpack(res.second, scratchpad.get());

            BOOST_CHECK_EQUAL(data.second, size);
            BOOST_CHECK_EQUAL(memcmp(data.first.get(), scratchpad.get(), size), 0);
        }

        // let us unwrap everything and check that it is there!
        for(size_t i = 0; i < 1024*10; ++i) {
            auto data = rand_data(i + seed, 20);
            auto size = set.unpack(ids[i], scratchpad.get());

            BOOST_CHECK_EQUAL(data.second, size);
            BOOST_CHECK_EQUAL(memcmp(data.first.get(), scratchpad.get(), size), 0);
            
            auto key = set.unpack(ids[i]);
            BOOST_CHECK_EQUAL(data.second, key.size());
            BOOST_CHECK_EQUAL(memcmp(data.first.get(), key.data(), size), 0);
        }
    }
}




struct type_t {
    char _a;
    int _b;
    char _c;
    int _d;
    bool operator==(const type_t& other) const
    {
        return _a == other._a && _b == other._b && _c == other._c && _d == other._d;
    }
    friend std::ostream& operator<<(std::ostream& os, const type_t& el)
    {
        std::cerr << (int)el._a << ", " << el._b << ", " << (int)el._c << ", " << el._d;
        return os;
    }
};

template<>
struct ptrie::byte_iterator<type_t>
{
    static constexpr uchar& access(type_t* data, size_t id)
    {
        auto el = id / element_size();
        id = id % element_size();
        assert(id < element_size());

        switch(id){
        case 0:
            return (uchar&)data[el]._a;
        case 1:
        case 2:
        case 3:
        case 4:
            return ((uchar*)&data[el]._b)[id-1];
        case 5:
            return (uchar&)data[el]._c;
        default:
            return ((uchar*)&data[el]._d)[id-6];
        }
    }

    static constexpr const uchar& const_access(const type_t* data, size_t id)
    {
        return access(const_cast<type_t*>(data), id);
    }

    static constexpr size_t element_size()
    {
        return sizeof(char)*2+sizeof(int)*2;
    }
    
    static constexpr bool continious(){
        return false;
    }
};


BOOST_AUTO_TEST_CASE(ComplexType1)
{
    std::cerr << "ComplexType1" << std::endl;
    for(size_t seed = 1337; seed < (1337+10); ++seed) {
        set_stable<type_t> set;
        vector<size_t> ids;
        type_t scratchpad;
        for(size_t i = 0; i < 1024*10; ++i) {
            srand(i + seed);
            type_t test({(char)rand(), (int)rand(), (char)rand(), (int)rand()});
            auto res = set.insert(test);
            BOOST_CHECK(res.first);
            ids.push_back(res.second);
            auto size = set.unpack(res.second, &scratchpad);

            BOOST_CHECK_EQUAL(size_t{1}, size);
            BOOST_CHECK_EQUAL(test, scratchpad);
        }

        // let us unwrap everything and check that it is there!
        for(size_t i = 0; i < 1024*10; ++i) {
            srand(i + seed);
            type_t test({(char)rand(), (int)rand(), (char)rand(), (int)rand()});
            auto size = set.unpack(ids[i], &scratchpad);

            BOOST_CHECK_EQUAL(size_t{1}, size);
            BOOST_CHECK_EQUAL(test, scratchpad);
            
            auto key = set.unpack(ids[i]);
            BOOST_CHECK_EQUAL(size_t{1}, key.size());
            BOOST_CHECK(key.back() == test);
        }
    }
}


BOOST_AUTO_TEST_CASE(ComplexType2)
{
    std::cerr << "ComplexType2" << std::endl;
    for(size_t seed = 1337; seed < (1337+10); ++seed) {
        set_stable<type_t, size_t,9> set;
        vector<size_t> ids;
        type_t scratchpad;
        for(size_t i = 0; i < 1024*10; ++i) {
            srand(i + seed);
            type_t test({(char)rand(), (int)rand(), (char)rand(), (int)rand()});
            auto res = set.insert(test);
            BOOST_REQUIRE(res.first);
            ids.push_back(res.second);
            auto size = set.unpack(res.second, &scratchpad);

            BOOST_REQUIRE_EQUAL(size_t{1}, size);
            BOOST_REQUIRE_EQUAL(test, scratchpad);
        }

        // let us unwrap everything and check that it is there!
        for(size_t i = 0; i < 1024*10; ++i) {
            srand(i + seed);
            type_t test({(char)rand(), (int)rand(), (char)rand(), (int)rand()});
            auto size = set.unpack(ids[i], &scratchpad);

            BOOST_CHECK_EQUAL(size_t{1}, size);
            BOOST_CHECK_EQUAL(test, scratchpad);
            
            auto key = set.unpack(ids[i]);
            BOOST_CHECK_EQUAL(size_t{1}, key.size());
            BOOST_CHECK(key.back() == test);
        }
    }
}

BOOST_AUTO_TEST_CASE(ComplexType1Vector)
{
    std::cerr << "ComplexType1Vector" << std::endl;
    for(size_t seed = 1337; seed < (1337+10); ++seed) {
        set_stable<type_t> set;
        vector<size_t> ids;
        std::vector<type_t> scratchpad(10);
        for(size_t i = 0; i < 1024*10; ++i) {
            srand(i + seed);
            std::vector<type_t> test(10);
            for(size_t i = 0; i < 10; ++i)
                test[i] = type_t{(char)rand(), (int)rand(), (char)rand(), (int)rand()};
            auto res = set.insert(test);
            BOOST_REQUIRE(res.first);
            ids.push_back(res.second);
            auto size = set.unpack(res.second, scratchpad.data());

            BOOST_REQUIRE_EQUAL(size_t{10}, size);
            BOOST_REQUIRE(std::equal(test.begin(), test.end(), scratchpad.begin()));
        }

        // let us unwrap everything and check that it is there!
        for(size_t i = 0; i < 1024*10; ++i) {
            srand(i + seed);
            std::vector<type_t> test(10);
            for(size_t i = 0; i < 10; ++i)
                test[i] = type_t{(char)rand(), (int)rand(), (char)rand(), (int)rand()};
            auto size = set.unpack(ids[i], scratchpad.data());

            BOOST_CHECK_EQUAL(size_t{10}, size);
            BOOST_CHECK(std::equal(test.begin(), test.end(), scratchpad.begin()));
            
            auto key = set.unpack(ids[i]);
            BOOST_CHECK_EQUAL(size_t{10}, key.size());
            BOOST_CHECK(std::equal(test.begin(), test.end(), key.begin()));
        }
    }
}

BOOST_AUTO_TEST_CASE(ComplexType2Vector)
{
    std::cerr << "ComplexType2Vector" << std::endl;
    for(size_t seed = 1337; seed < (1337+10); ++seed) {
        set_stable<type_t, size_t,9> set;
        vector<size_t> ids;
        std::vector<type_t> scratchpad(10);
        for(size_t i = 0; i < 1024*10; ++i) {
            srand(i + seed);
            std::vector<type_t> test(10);
            for(size_t i = 0; i < 10; ++i)
                test[i] = type_t{(char)rand(), (int)rand(), (char)rand(), (int)rand()};
            auto res = set.insert(test);
            BOOST_REQUIRE(res.first);
            ids.push_back(res.second);
            auto size = set.unpack(res.second, scratchpad.data());

            BOOST_REQUIRE_EQUAL(size_t{10}, size);
            BOOST_REQUIRE(std::equal(test.begin(), test.end(), scratchpad.begin()));
        }

        // let us unwrap everything and check that it is there!
        for(size_t i = 0; i < 1024*10; ++i) {
            srand(i + seed);
            std::vector<type_t> test(10);
            for(size_t i = 0; i < 10; ++i)
                test[i] = type_t{(char)rand(), (int)rand(), (char)rand(), (int)rand()};
            auto size = set.unpack(ids[i], scratchpad.data());

            BOOST_CHECK_EQUAL(size_t{10}, size);
            BOOST_CHECK(std::equal(test.begin(), test.end(), scratchpad.begin()));
            
            auto key = set.unpack(ids[i]);
            BOOST_CHECK_EQUAL(size_t{10}, key.size());
            BOOST_CHECK(std::equal(test.begin(), test.end(), key.begin()));
        }
    }
}

BOOST_AUTO_TEST_CASE(SimpleCopy)
{
    set_stable<size_t> set;
    for(size_t i = 0; i < 100000; ++i)
    {
        set.insert(i);
    }
    {
        auto cpy = set;
        size_t i = 0;
        for(; i < 100000; ++i)
            BOOST_REQUIRE(cpy.exists(i).first);
        for(; i < 200000; ++i)
            BOOST_REQUIRE(!cpy.exists(i).first);
    }
}
