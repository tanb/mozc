// Copyright 2010-2011, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "dictionary/dictionary_mock.h"

#include "base/base.h"
#include "base/util.h"
#include "converter/node.h"
#include "dictionary/dictionary_token.h"
#include "testing/base/public/googletest.h"
#include "testing/base/public/gunit.h"
namespace mozc {

namespace {
class TestNodeAllocator : public NodeAllocatorInterface {
 public:
  TestNodeAllocator() {}
  virtual ~TestNodeAllocator() {
    for (size_t i = 0; i < nodes_.size(); ++i) {
      delete nodes_[i];
    }
    nodes_.clear();
  }

  Node *NewNode() {
    Node *node = new Node;
    CHECK(node);
    node->Init();
    nodes_.push_back(node);
    return node;
  }

 private:
  vector<Node *> nodes_;
};
}  // namespace

class DictionaryMockTest : public testing::Test {
 protected:
  void SetUp() {
    mock_.reset(new DictionaryMock);
    DictionaryFactory::SetDictionary(mock_.get());
  }

  void TearDown() {
    DictionaryFactory::SetDictionary(NULL);
  }

  DictionaryMock *GetMock() {
    return mock_.get();
  }

  Token *CreateToken(const string &key, const string &value);
  bool SearchMatchingNode(const string &key,
                          const string &value,
                          uint32 attributes,
                          const Node *node);
  void DeleteNodes(Node *node);
  scoped_ptr<DictionaryMock> mock_;
};

bool DictionaryMockTest::SearchMatchingNode(const string &key,
                                            const string &value,
                                            uint32 attributes,
                                            const Node *node) {
  CHECK(node);
  while (node) {
    if (node->key == key && node->value == value &&
        node->attributes == attributes) {
      return true;
    }
    node = node->bnext;
  }
  return false;
}

Token *DictionaryMockTest::CreateToken(const string &key, const string &value) {
  Token *token = new Token;
  memset(token, 0, sizeof(*token));
  token->key = key;
  token->value = value;
  return token;
}

void DictionaryMockTest::DeleteNodes(Node *node) {
  while (node) {
    Node *next_node = node->bnext;
    delete node;
    node = next_node;
  }
}

TEST_F(DictionaryMockTest, test_prefix) {
  DictionaryInterface *dic = DictionaryFactory::GetDictionary();

  // "は"
  const string k0 = "\xe3\x81\xaf";
  // "はひふへほ"
  const string k1 = "\xe3\x81\xaf\xe3\x81\xb2\xe3\x81\xb5\xe3\x81\xb8\xe3\x81"
                    "\xbb";

  vector<Token> tokens;
  scoped_ptr<Token> t0(CreateToken(k0, "v0"));
  scoped_ptr<Token> t1(CreateToken(k1, "v1"));

  tokens.push_back(*t0.get());
  tokens.push_back(*t1.get());

  vector<Token>::iterator it = tokens.begin();
  for (; it != tokens.end(); ++it) {
    GetMock()->AddLookupPrefix(it->key, it->key, it->value, 0);
  }

  Node *node = dic->LookupPrefix(k1.c_str(), k1.size(), NULL);
  CHECK(node) << "no nodes found";
  EXPECT_TRUE(SearchMatchingNode(t1->key, t1->value, 0, node))
              << "Failed to find " << t1->key;
  EXPECT_TRUE(SearchMatchingNode(t0->key, t0->value, 0, node))
              << "Failed to find " << t0->key;
  DeleteNodes(node);
}

TEST_F(DictionaryMockTest, test_reverse) {
  DictionaryInterface *dic = DictionaryFactory::GetDictionary();

  // "今"/"いま"
  const string k0 = "\xE4\xBB\x8A";
  const string v0 = "\xE3\x81\x84\xE3\x81\xBE";
  // "今日"/"きょう"
  const string k1 = "\xE4\xBB\x8A\xE6\x97\xA5";
  const string v1 = "\xE3\x81\x8D\xE3\x82\x87\xE3\x81\x86";

  vector<Token> tokens;
  scoped_ptr<Token> t0(CreateToken(k0, v0));
  scoped_ptr<Token> t1(CreateToken(k1, v1));

  tokens.push_back(*t0.get());
  tokens.push_back(*t1.get());

  vector<Token>::iterator it = tokens.begin();
  for (; it != tokens.end(); ++it) {
    GetMock()->AddLookupReverse(it->key, it->key, it->value, 0);
  }

  Node *node = dic->LookupReverse(k1.c_str(), k1.size(), NULL);
  CHECK(node) << "No nodes found";
  EXPECT_TRUE(SearchMatchingNode(t1->key, t1->value, 0, node))
              << "Failed to find: " << t1->key;
  EXPECT_TRUE(SearchMatchingNode(t0->key, t0->value, 0, node))
              << "Failed to find: " << t0->key;
  DeleteNodes(node);
}

TEST_F(DictionaryMockTest, test_predictive) {
  DictionaryInterface *dic = DictionaryFactory::GetDictionary();
  // "は"
  const string k0 = "\xe3\x81\xaf";
  // "はひふ"
  const string k1 = "\xe3\x81\xaf\xe3\x81\xb2\xe3\x81\xb5";
  // "はひふへほ"
  const string k2 = "\xe3\x81\xaf\xe3\x81\xb2\xe3\x81\xb5\xe3\x81\xb8\xe3\x81"
                    "\xbb";

  vector<Token> tokens;
  scoped_ptr<Token> t1(CreateToken(k1, "v0"));
  scoped_ptr<Token> t2(CreateToken(k2, "v1"));
  tokens.push_back(*t1.get());
  tokens.push_back(*t2.get());

  vector<Token>::iterator it = tokens.begin();
  for (; it != tokens.end(); ++it) {
    GetMock()->AddLookupPredictive(k0, it->key, it->value, 0);
  }

  Node *node = dic->LookupPredictive(k0.c_str(), k0.size(), NULL);
  CHECK(node) << "no nodes found";
  EXPECT_TRUE(SearchMatchingNode(t1->key, t1->value, 0, node))
              << "Failed to find " << t1->key;
  EXPECT_TRUE(SearchMatchingNode(t2->key, t2->value, 0, node))
              << "Failed to find " << t2->key;
  DeleteNodes(node);
}

}  // namespace mozc