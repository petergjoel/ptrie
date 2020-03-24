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
#define BOOST_TEST_MODULE PTrieSet
#include <boost/test/unit_test.hpp>

#include <ptrie/ptrie.h>
#include "utils.h"

using namespace ptrie;

BOOST_AUTO_TEST_CASE(EmptyTest)
{
    set<> set;
    try_insert(set,
               [](size_t i)
               {
                   auto data = std::make_unique<unsigned char[]>(0);
                   return std::make_pair(std::move(data), 0);
               },
               1);
}

BOOST_AUTO_TEST_CASE(InsertByte)
{
    set<> set;
    try_insert(set,
               [](size_t i)
               {
                   auto data = std::make_unique<unsigned char[]>(1);
                   data[0] = (uchar) i;
                   return std::make_pair(std::move(data), 1);
               },
               256);
}

BOOST_AUTO_TEST_CASE(InsertByteSplit)
{
    set<unsigned char, 128, 6> set;
    try_insert(set,
               [](size_t i)
               {
                   auto data = std::make_unique<unsigned char[]>(1);
                   data[0] = (uchar) i;
                   return std::make_pair(std::move(data), 1);
               },
               256);
}

BOOST_AUTO_TEST_CASE(HeapTest)
{
    set<unsigned char, sizeof (size_t) + 1 > set;
    try_insert(set,
               [](size_t i)
               {
                   auto data = std::make_unique<unsigned char[]>(sizeof (size_t));
                   memcpy(data.get(), &i, sizeof (size_t));
                   return std::make_pair(std::move(data), sizeof (size_t));
               },
               1024);
}

BOOST_AUTO_TEST_CASE(InsertMill)
{
    set<> set;
    try_insert(set,
               [](size_t i)
               {
                   auto data = std::make_unique<unsigned char[]>(sizeof (size_t));
                   memcpy(data.get(), &i, sizeof (size_t));
                   return std::make_pair(std::move(data), sizeof (size_t));
               },
               1024 * 1024);
}

BOOST_AUTO_TEST_CASE(PseudoRand1)
{
    for(size_t seed = 0; seed < 10; ++ seed) {
        set<> set;
        try_insert(set,
                  [seed](size_t i) {
                      return rand_data(seed + i, 256);
                  },
                  1024 * 10);
    }
}

BOOST_AUTO_TEST_CASE(PseudoRand2)
{
    for(size_t seed = 0; seed < 10; ++ seed) {
        set<> set;
        try_insert(set,
                  [seed](size_t i) {
                     return rand_data(seed + i, 16);
                  },
                  1024 * 10);
    }
}


BOOST_AUTO_TEST_CASE(PseudoRandSplitHeap)
{
    for(size_t seed = 42; seed < (42+10); ++seed) {
        set<unsigned char,sizeof(size_t)+1, 6> set;
        try_insert(set,
                  [seed](size_t i) {
                     return rand_data(seed + i, 16);
                  },
                  1024 * 10);
    }
}

BOOST_AUTO_TEST_CASE(SimpleCopy)
{
    set<size_t> set;
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