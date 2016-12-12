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
#define BOOST_TEST_MODULE PTrieSet
#include <boost/test/unit_test.hpp>

#include <ptrie.h>
#include <stdlib.h>

using namespace ptrie;


void tryInsert(auto& trie, auto generator, size_t N)
{
    for(size_t i = 0; i < N; ++i)
    {
        binarywrapper_t data = generator(i);
        auto exists = trie.exists(data.raw(), data.size());
        BOOST_REQUIRE_MESSAGE(!exists.first,
                              "FAILED ON INSERT " << i << " BIN " << data << " ");

        auto inserted = trie.insert(data.raw(), data.size());
        BOOST_REQUIRE_MESSAGE(inserted.first,
                              "EXIST FAILED FOR " << i << " BIN " << data << " ");
        data.release();
    }

    for(size_t i = 0; i < N; ++i) {
        binarywrapper_t data = generator(i);
        auto exists = trie.exists(data.raw(), data.size());
        BOOST_REQUIRE_MESSAGE(exists.first,
                              "POST EXIST CHECK FAILED FOR " << i << " BIN " << data << " ");
        data.release();
    }

};

BOOST_AUTO_TEST_CASE(EmptyTest)
{
    set<> set;
    tryInsert(set,
              [](size_t i){
                  binarywrapper_t data;
                  return data;
              },
            1);
}

BOOST_AUTO_TEST_CASE(InsertByte)
{
    set<> set;
    tryInsert(set,
              [](size_t i){
                  binarywrapper_t data(8);
                  data.raw()[0] = (uchar)i;
                  return data;
              },
            256);
}

BOOST_AUTO_TEST_CASE(InsertByteSplit)
{
    set<128,2> set;
    tryInsert(set,
              [](size_t i){
                  binarywrapper_t data(8);
                  data.raw()[0] = (uchar)i;
                  return data;
              },
            256);
}

BOOST_AUTO_TEST_CASE(HeapTest)
{
    set<2> set;
    tryInsert(set,
              [](size_t i){
                  binarywrapper_t data;
                  data.copy((uchar*)&i, sizeof(size_t));
                  return data;
              },
            1024);
}

BOOST_AUTO_TEST_CASE(InsertMill)
{
    set<> set;
    tryInsert(set,
              [](size_t i){
                  binarywrapper_t data;
                  data.copy((uchar*)&i, sizeof(size_t));
                  return data;
              },
            1024*1024);
}

BOOST_AUTO_TEST_CASE(PseudoRand1)
{
    for(size_t seed = 0; seed < 10; ++ seed) {
        set<> set;
        tryInsert(set,
                  [seed](size_t i) {
                      srand(seed + i);
                      // pick size between 0 and 256
                      size_t size = sizeof(size_t) +
                                    rand() % (256 - sizeof(size_t));

                      binarywrapper_t data(size * 8);
                      // fill in random data
                      for (size_t j = 0; j < size; ++j) {
                          data.raw()[j] = (uchar) rand();
                      }
                      // make sure everything is unique
                      for (size_t j = 1; j <= sizeof(size_t); ++j) {
                          data.raw()[size - j] = ((uchar *) &i)[j - 1];
                      }
                      return data;
                  },
                  1024 * 10);
    }
}

BOOST_AUTO_TEST_CASE(PseudoRand2)
{
    for(size_t seed = 0; seed < 10; ++ seed) {
        set<> set;
        tryInsert(set,
                  [seed](size_t i) {
                      srand(seed + i);
                      // pick size between 0 and 256
                      size_t size = sizeof(size_t) +
                                    rand() % (20 - sizeof(size_t));

                      binarywrapper_t data(size * 8);
                      // fill in random data
                      for (size_t j = 0; j < size; ++j) {
                          data.raw()[j] = (uchar) rand();
                      }
                      // make sure everything is unique
                      for (size_t j = 1; j <= sizeof(size_t); ++j) {
                          data.raw()[size - j] = ((uchar *) &i)[j - 1];
                      }
                      return data;
                  },
                  1024 * 10);
    }
}

