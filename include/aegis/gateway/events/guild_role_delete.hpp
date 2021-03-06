//
// guild_role_delete.hpp
// *********************
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/snowflake.hpp"

namespace aegis
{

namespace gateway
{

namespace events
{

/// Sent when a guild role was deleted
struct guild_role_delete
{
    shards::shard & shard; /**< Reference to shard object this message came from */
    snowflake guild_id; /**< Snowflake of guild */
    snowflake role_id; /**< Snowflake of role that was deleted */
};

}

}

}
