#pragma once

#include "cryptonote_basic/blobdatatype.h"
#include "cryptonote_protocol/enums.h"
#include "span.h"

namespace cryptonote
{
  struct i_core_events
  {
    virtual ~i_core_events() noexcept
    {}

    virtual void on_transactions_relayed(epee::span<const cryptonote::blobdata> tx_blobs, relay_method tx_relay) = 0;
  };
}
