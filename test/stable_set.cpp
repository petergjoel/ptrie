/* 
 * Copyright (C) 2016  Peter G. Jensen <root@petergjoel.dk>
 * 
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
// Created by Peter G. Jensen on 12/9/16.
#define BOOST_TEST_MODULE PTrieStableSet
#include <boost/test/unit_test.hpp>
#include <ptrie/ptrie_stable.h>
#include <vector>
#include "utils.h"

using namespace ptrie;
using namespace std;

BOOST_AUTO_TEST_CASE(PseudoRand1)
{
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
        }
    }
}

BOOST_AUTO_TEST_CASE(PseudoRandSplitHeap)
{
    for(size_t seed = 42; seed < (42+10); ++seed) {
        set_stable<unsigned char, sizeof(size_t)+1, 4> set;
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
        }
    }
}

