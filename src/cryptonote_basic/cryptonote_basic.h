// Copyright (c) 2018-2022, The Arqma Network
// Copyright (c) 2014-2018, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#include <boost/variant.hpp>
#include <boost/functional/hash/hash.hpp>
#include <vector>
#include <cstring>  // memcmp
#include <sstream>
#include <atomic>
#include "serialization/variant.h"
#include "serialization/vector.h"
#include "serialization/binary_archive.h"
#include "serialization/json_archive.h"
#include "serialization/debug_archive.h"
#include "serialization/crypto.h"
#include "serialization/keyvalue_serialization.h" // epee named serialization
#include "cryptonote_config.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "misc_language.h"
#include "ringct/rctTypes.h"
#include "device/device.hpp"
#include "txtypes.h"

namespace cryptonote
{
  typedef std::vector<crypto::signature> ring_signature;


  /* outputs */

  struct txout_to_script
  {
    std::vector<crypto::public_key> keys;
    std::vector<uint8_t> script;

    BEGIN_SERIALIZE_OBJECT()
      FIELD(keys)
      FIELD(script)
    END_SERIALIZE()
  };

  struct txout_to_scripthash
  {
    crypto::hash hash;
  };

  struct txout_to_key
  {
    txout_to_key() = default;
    txout_to_key(const crypto::public_key &_key) : key(_key) { }
    crypto::public_key key;
  };


  /* inputs */

  struct txin_gen
  {
    size_t height;

    BEGIN_SERIALIZE_OBJECT()
      VARINT_FIELD(height)
    END_SERIALIZE()
  };

  struct txin_to_script
  {
    crypto::hash prev;
    size_t prevout;
    std::vector<uint8_t> sigset;

    BEGIN_SERIALIZE_OBJECT()
      FIELD(prev)
      VARINT_FIELD(prevout)
      FIELD(sigset)
    END_SERIALIZE()
  };

  struct txin_to_scripthash
  {
    crypto::hash prev;
    size_t prevout;
    txout_to_script script;
    std::vector<uint8_t> sigset;

    BEGIN_SERIALIZE_OBJECT()
      FIELD(prev)
      VARINT_FIELD(prevout)
      FIELD(script)
      FIELD(sigset)
    END_SERIALIZE()
  };

  struct txin_to_key
  {
    uint64_t amount;
    std::vector<uint64_t> key_offsets;
    crypto::key_image k_image;      // double spending protection

    BEGIN_SERIALIZE_OBJECT()
      VARINT_FIELD(amount)
      FIELD(key_offsets)
      FIELD(k_image)
    END_SERIALIZE()
  };


  typedef boost::variant<txin_gen, txin_to_script, txin_to_scripthash, txin_to_key> txin_v;

  typedef boost::variant<txout_to_script, txout_to_scripthash, txout_to_key> txout_target_v;

  //typedef std::pair<uint64_t, txout> out_t;
  struct tx_out
  {
    uint64_t amount;
    txout_target_v target;

    BEGIN_SERIALIZE_OBJECT()
      VARINT_FIELD(amount)
      FIELD(target)
    END_SERIALIZE()


  };

  template<typename T> static inline unsigned int getpos(T &ar) { return 0; }
  template<> inline unsigned int getpos(binary_archive<true> &ar) { return ar.stream().tellp(); }
  template<> inline unsigned int getpos(binary_archive<false> &ar) { return ar.stream().tellg(); }

  class transaction_prefix
  {

  public:
    static char const* version_to_string(txversion v);
    static char const* type_to_string(txtype tx_type);

    static constexpr txversion get_min_version_for_hf(uint8_t hard_fork_version);
    static constexpr txversion get_max_version_for_hf(uint8_t hard_fork_version);
    static constexpr txtype get_max_type_for_hf(uint8_t hard_fork_version);

    // tx information
    txversion version;
    txtype tx_type;
    uint8_t hard_fork_version;

    bool is_transfer() const { return (tx_type == txtype::standard || tx_type == txtype::stake); }

    uint64_t unlock_time;  // number of block (or time), used as a limitation like: spend this tx not early then block/time

    std::vector<txin_v> vin;
    std::vector<tx_out> vout;
    //extra
    std::vector<uint8_t> extra;

    std::vector<uint64_t> output_unlock_times;

    BEGIN_SERIALIZE()
      ENUM_FIELD(version, version >= txversion::v1 && version < txversion::_count);
      if(version >= txversion::v3)
      {
        ENUM_FIELD_N("tx_type", tx_type, tx_type < txtype::_count);
        FIELD(output_unlock_times)
      }
      VARINT_FIELD(unlock_time)
      FIELD(vin)
      FIELD(vout)
      if(version >= txversion::v3 && vout.size() != output_unlock_times.size())
        return false;
      FIELD(extra)
    END_SERIALIZE()

    transaction_prefix() { set_null(); }
    void set_null();

    uint64_t get_unlock_time(size_t out_index) const
    {
      if(version >= txversion::v3)
      {
        if(out_index >= output_unlock_times.size())
        {
          LOG_ERROR("Tried to get unlock time of a v3 transaction with missing output unlock time");
          return unlock_time;
        }
        return output_unlock_times[out_index];
      }
      return unlock_time;
    }

  };

  class transaction final : public transaction_prefix
  {
  private:
    // hash cash
    mutable std::atomic<bool> hash_valid;
    mutable std::atomic<bool> blob_size_valid;

  public:
    std::vector<std::vector<crypto::signature>> signatures; //count signatures  always the same as inputs count
    rct::rctSig rct_signatures;

    // hash cash
    mutable crypto::hash hash;
    mutable size_t blob_size;

    bool pruned;

    std::atomic<unsigned int> unprunable_size;
    std::atomic<unsigned int> prefix_size;

    transaction() { set_null(); }
    transaction(const transaction &t);
    transaction(transaction&& t);
    transaction& operator=(const transaction& t);
    transaction& operator=(transaction&& t);
    ~transaction() = default;
    void set_null();
    void invalidate_hashes();
    bool is_hash_valid() const { return hash_valid.load(std::memory_order_acquire); }
    void set_hash_valid(bool v) const { hash_valid.store(v,std::memory_order_release); }
    bool is_blob_size_valid() const { return blob_size_valid.load(std::memory_order_acquire); }
    void set_blob_size_valid(bool v) const { blob_size_valid.store(v,std::memory_order_release); }
    void set_hash(const crypto::hash &h) { hash = h; set_hash_valid(true); }
    void set_blob_size(size_t sz) { blob_size = sz; set_blob_size_valid(true); }

    BEGIN_SERIALIZE_OBJECT()
      if (!typename Archive<W>::is_saving())
      {
        set_hash_valid(false);
        set_blob_size_valid(false);
      }

      const unsigned int start_pos = getpos(ar);

      FIELDS(*static_cast<transaction_prefix *>(this))

      if (std::is_same<Archive<W>, binary_archive<W>>())
        prefix_size = getpos(ar) - start_pos;

      if (version == txversion::v1)
      {
        if (std::is_same<Archive<W>, binary_archive<W>>())
          unprunable_size = getpos(ar) - start_pos;

        ar.tag("signatures");
        ar.begin_array();
        PREPARE_CUSTOM_VECTOR_SERIALIZATION(vin.size(), signatures);
        bool signatures_not_expected = signatures.empty();
        if (!signatures_not_expected && vin.size() != signatures.size())
          return false;

        if (!pruned) for (size_t i = 0; i < vin.size(); ++i)
        {
          size_t signature_size = get_signature_size(vin[i]);
          if (signatures_not_expected)
          {
            if (0 == signature_size)
              continue;
            else
              return false;
          }

          PREPARE_CUSTOM_VECTOR_SERIALIZATION(signature_size, signatures[i]);
          if (signature_size != signatures[i].size())
            return false;

          FIELDS(signatures[i]);

          if (vin.size() - i > 1)
            ar.delimit_array();
        }
        ar.end_array();
      }
      else
      {
        ar.tag("rct_signatures");
        if (!vin.empty())
        {
          ar.begin_object();
          bool r = rct_signatures.serialize_rctsig_base(ar, vin.size(), vout.size());
          if (!r || !ar.stream().good()) return false;
          ar.end_object();

          if (std::is_same<Archive<W>, binary_archive<W>>())
            unprunable_size = getpos(ar) - start_pos;

          if (!pruned && rct_signatures.type != rct::RCTTypeNull)
          {
            ar.tag("rctsig_prunable");
            ar.begin_object();
            r = rct_signatures.p.serialize_rctsig_prunable(ar, rct_signatures.type, vin.size(), vout.size(),
                vin.size() > 0 && vin[0].type() == typeid(txin_to_key) ? boost::get<txin_to_key>(vin[0]).key_offsets.size() - 1 : 0);
            if (!r || !ar.stream().good()) return false;
            ar.end_object();
          }
        }
      }
      if (!typename Archive<W>::is_saving())
        pruned = false;
    END_SERIALIZE()

    template<bool W, template <bool> class Archive>
    bool serialize_base(Archive<W> &ar)
    {
      FIELDS(*static_cast<transaction_prefix *>(this))

      if (version == txversion::v1)
      {
      }
      else
      {
        ar.tag("rct_signatures");
        if (!vin.empty())
        {
          ar.begin_object();
          bool r = rct_signatures.serialize_rctsig_base(ar, vin.size(), vout.size());
          if (!r || !ar.stream().good()) return false;
          ar.end_object();
        }
      }
      if (!typename Archive<W>::is_saving())
        pruned = true;
      return ar.stream().good();
    }

  private:
    static size_t get_signature_size(const txin_v& tx_in);
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct block_header
  {
    uint8_t major_version = {};
    uint8_t minor_version = {};  // now used as a voting mechanism, rather than how this particular block is built
    uint64_t timestamp;
    crypto::hash prev_id;
    uint32_t nonce;

    BEGIN_SERIALIZE()
      VARINT_FIELD(major_version)
      VARINT_FIELD(minor_version)
      VARINT_FIELD(timestamp)
      FIELD(prev_id)
      FIELD(nonce)
    END_SERIALIZE()
  };

  struct block: public block_header
  {
  private:
    // hash cash
    mutable std::atomic<bool> hash_valid{false};
    void copy_hash(const block &b) { bool v = b.is_hash_valid(); hash = b.hash; set_hash_valid(v); }

  public:
    block() = default;
    block(const block& b);
    block(block&& b);
    block& operator=(const block& b);
    block& operator=(block&& b);
    void invalidate_hashes() { set_hash_valid(false); }
    bool is_hash_valid() const;
    void set_hash_valid(bool v) const;

    transaction miner_tx;
    std::vector<crypto::hash> tx_hashes;

    // hash cash
    mutable crypto::hash hash;

    BEGIN_SERIALIZE_OBJECT()
      if (!typename Archive<W>::is_saving())
        set_hash_valid(false);

      FIELDS(*static_cast<block_header *>(this))
      FIELD(miner_tx)
      FIELD(tx_hashes)
      if(tx_hashes.size() > config::tx_settings::MAX_TRANSACTIONS_IN_BLOCK)
        return false;
    END_SERIALIZE()
  };


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct account_public_address
  {
    crypto::public_key m_spend_public_key;
    crypto::public_key m_view_public_key;

    BEGIN_SERIALIZE_OBJECT()
      FIELD(m_spend_public_key)
      FIELD(m_view_public_key)
    END_SERIALIZE()

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE(m_spend_public_key)
      KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE(m_view_public_key)
    END_KV_SERIALIZE_MAP()

    bool operator==(const account_public_address& rhs) const
    {
      return m_spend_public_key == rhs.m_spend_public_key &&
             m_view_public_key == rhs.m_view_public_key;
    }

    bool operator!=(const account_public_address& rhs) const
    {
      return !(*this == rhs);
    }
  };

  struct keypair
  {
    crypto::public_key pub;
    crypto::secret_key sec;

    static inline keypair generate(hw::device &hwdev)
    {
      keypair k;
      hwdev.generate_keys(k.pub, k.sec);
      return k;
    }
  };

  constexpr txversion transaction_prefix::get_min_version_for_hf(uint8_t hard_fork_version)
  {
    if (hard_fork_version < 14)
      return txversion::v1;
    if (hard_fork_version < 16)
      return txversion::v2;

    return txversion::v3;
  }

  constexpr txversion transaction_prefix::get_max_version_for_hf(uint8_t hard_fork_version)
  {
    if (hard_fork_version < 16)
      return txversion::v2;

    return txversion::v3;
  }

  constexpr txtype transaction_prefix::get_max_type_for_hf(uint8_t hard_fork_version)
  {
    if (hard_fork_version < 16)
      return txtype::standard;

    return txtype::stake;
  }

  inline const char* transaction_prefix::version_to_string(txversion v)
  {
    switch(v)
    {
      case txversion::v1: return "1";
      case txversion::v2: return "2";
      case txversion::v3: return "3";
      default: assert(false); return "unhandled_version";
    }
  }

  inline const char* transaction_prefix::type_to_string(txtype tx_type)
  {
    switch(tx_type)
    {
      case txtype::standard: return "standard";
      case txtype::state_change: return "state_change";
      case txtype::key_image_unlock: return "key_image_unlock";
      case txtype::stake: return "stake";
      default: assert(false); return "unhandled_type";
    }
  }

  inline std::ostream &operator<<(std::ostream &os, txtype tx_type) {
    return os << transaction::type_to_string(tx_type);
  }

  inline std::ostream &operator<<(std::ostream &os, txversion v) {
    return os << transaction::version_to_string(v);
  }
}

namespace std {
  template <>
  struct hash<cryptonote::account_public_address>
  {
    std::size_t operator()(const cryptonote::account_public_address& addr) const
    {
      // https://stackoverflow.com/a/17017281
      size_t res = 17;
      res = res * 31 + hash<crypto::public_key>()(addr.m_spend_public_key);
      res = res * 31 + hash<crypto::public_key>()(addr.m_view_public_key);
      return res;
    }
  };
}

BLOB_SERIALIZER(cryptonote::txout_to_key);
BLOB_SERIALIZER(cryptonote::txout_to_scripthash);

VARIANT_TAG(binary_archive, cryptonote::txin_gen, 0xff);
VARIANT_TAG(binary_archive, cryptonote::txin_to_script, 0x0);
VARIANT_TAG(binary_archive, cryptonote::txin_to_scripthash, 0x1);
VARIANT_TAG(binary_archive, cryptonote::txin_to_key, 0x2);
VARIANT_TAG(binary_archive, cryptonote::txout_to_script, 0x0);
VARIANT_TAG(binary_archive, cryptonote::txout_to_scripthash, 0x1);
VARIANT_TAG(binary_archive, cryptonote::txout_to_key, 0x2);
VARIANT_TAG(binary_archive, cryptonote::transaction, 0xcc);
VARIANT_TAG(binary_archive, cryptonote::block, 0xbb);

VARIANT_TAG(json_archive, cryptonote::txin_gen, "gen");
VARIANT_TAG(json_archive, cryptonote::txin_to_script, "script");
VARIANT_TAG(json_archive, cryptonote::txin_to_scripthash, "scripthash");
VARIANT_TAG(json_archive, cryptonote::txin_to_key, "key");
VARIANT_TAG(json_archive, cryptonote::txout_to_script, "script");
VARIANT_TAG(json_archive, cryptonote::txout_to_scripthash, "scripthash");
VARIANT_TAG(json_archive, cryptonote::txout_to_key, "key");
VARIANT_TAG(json_archive, cryptonote::transaction, "tx");
VARIANT_TAG(json_archive, cryptonote::block, "block");

VARIANT_TAG(debug_archive, cryptonote::txin_gen, "gen");
VARIANT_TAG(debug_archive, cryptonote::txin_to_script, "script");
VARIANT_TAG(debug_archive, cryptonote::txin_to_scripthash, "scripthash");
VARIANT_TAG(debug_archive, cryptonote::txin_to_key, "key");
VARIANT_TAG(debug_archive, cryptonote::txout_to_script, "script");
VARIANT_TAG(debug_archive, cryptonote::txout_to_scripthash, "scripthash");
VARIANT_TAG(debug_archive, cryptonote::txout_to_key, "key");
VARIANT_TAG(debug_archive, cryptonote::transaction, "tx");
VARIANT_TAG(debug_archive, cryptonote::block, "block");
