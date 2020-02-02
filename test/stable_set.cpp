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
        binarywrapper_t scratchpad((20+sizeof(size_t))*8);

        for(size_t i = 0; i < 1024*10; ++i) {
            binarywrapper_t data = rand_data(i + seed, 20);
            auto res = set.insert(data);
            BOOST_CHECK(res.first);
            ids.push_back(res.second);
            set.unpack(res.second, scratchpad.raw());

            // shallow copy
            binarywrapper_t shallow(scratchpad.raw(), data.size()*8);
            BOOST_CHECK_EQUAL(shallow.size(), data.size());
            BOOST_CHECK(shallow == data);
            data.release();
        }

        // let us unwrap everything and check that it is there!

        for(size_t i = 0; i < 1024*10; ++i) {
            binarywrapper_t data = rand_data(i + seed, 20);
            set.unpack(ids[i], scratchpad.raw());

            // shallow copy
            binarywrapper_t shallow(scratchpad.raw(), data.size()*8);
            BOOST_CHECK_EQUAL(shallow.size(), data.size());
            BOOST_CHECK(shallow == data);
            data.release();
        }
    }
}

BOOST_AUTO_TEST_CASE(PseudoRandSplitHeap)
{
    for(size_t seed = 42; seed < (42+10); ++seed) {
        set_stable<unsigned char, sizeof(size_t)+1, 4> set;
        vector<size_t> ids;
        binarywrapper_t scratchpad((20+sizeof(size_t))*8);

        for(size_t i = 0; i < 1024*10; ++i) {
            binarywrapper_t data = rand_data(i + seed, 20);
            auto res = set.insert(data);
            BOOST_CHECK(res.first);
            ids.push_back(res.second);
            set.unpack(res.second, scratchpad.raw());

            // shallow copy
            binarywrapper_t shallow(scratchpad.raw(), data.size()*8);
            BOOST_CHECK_EQUAL(shallow.size(), data.size());
            BOOST_CHECK(shallow == data);
            data.release();
        }

        // let us unwrap everything and check that it is there!

        for(size_t i = 0; i < 1024*10; ++i) {
            binarywrapper_t data = rand_data(i + seed, 20);
            set.unpack(ids[i], scratchpad.raw());

            // shallow copy
            binarywrapper_t shallow(scratchpad.raw(), data.size()*8);
            BOOST_CHECK_EQUAL(shallow.size(), data.size());
            BOOST_CHECK(shallow == data);
            data.release();
        }
    }
}

