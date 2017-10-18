//
// example.hpp
// aegis.cpp
//
// Copyright (c) 2017 Sara W (sara at xandium dot net)
//
// This file is part of aegis.cpp .
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#pragma once

#include <aegis.hpp>


namespace example_bot
{

using aegis::client;
using json = nlohmann::json;
using namespace std::placeholders;
using namespace aegis;


class example
{
public:
    example() = default;
    ~example() = default;

    template<typename Out>
    void split(const std::string &s, char delim, Out result)
    {
        std::stringstream ss;
        ss.str(s);
        std::string item;
        while (std::getline(ss, item, delim))
        {
            if (!item.empty())
                *(result++) = item;
        }
    }

    std::vector<std::string> split(const std::string &s, char delim)
    {
        std::vector<std::string> elems;
        split(s, delim, std::back_inserter(elems));
        return elems;
    }

    using c_inject = std::function<bool(json & msg, client & shard, Aegis & bot)>;

    // Functions you wish you hook into
    void inject(aegis::Aegis & bot)
    {
        bot.i_message_create = std::bind(&example::message_create, this, _1, _2, _3);
        bot.i_guild_create = std::bind(&example::guild_create, this, _1, _2, _3);
        bot.i_guild_delete = std::bind(&example::guild_delete, this, _1, _2, _3);
        bot.i_ready = std::bind(&example::ready, this, _1, _2, _3);
        bot.i_resumed = std::bind(&example::resumed, this, _1, _2, _3);
    }


    // All the hooks into the websocket stream
    // Your hooked functions take priority over the library for processing.
    // Returning a false makes the library skip handling the data when you are done. (except READY)
    bool typing_start(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool message_create(json & msg, client & shard, Aegis & bot)
    {
        json author = msg["d"]["author"];

        uint64_t userid = std::stoll(author["id"].get<std::string>());
        std::string username = author["username"];

        //test
        //auto[_ctime, _, _, _] = snowflake::c_get_all(userid);
        //auto[_time, _worker,_,_] = (snowflake(userid)).get_all();
        //test

        snowflake channel_id = std::stoll(msg["d"]["channel_id"].get<std::string>());
        snowflake message_id = std::stoll(msg["d"]["id"].get<std::string>());
        std::string content = msg["d"]["content"];

        std::string avatar = author["avatar"].is_string() ? author["avatar"] : "";
        uint16_t discriminator = std::stoi(author["discriminator"].get<std::string>());

        uint64_t msgid = std::stoll(msg["d"]["id"].get<std::string>());

        channel & _channel = bot.get_channel(channel_id);
        guild & _guild = _channel.get_guild();

        auto toks = split(content, ' ');
        if (toks.size() == 0)
            return true;
        if (toks[0] == "?info")
        {
            uint64_t guild_count = bot.m_guilds.size();
            uint64_t member_count = 0;
            uint64_t member_count_unique = bot.m_members.size();
            uint64_t member_online_count = 0;
            uint64_t member_idle_count = 0;
            uint64_t member_dnd_count = 0;
            uint64_t channel_count = bot.m_channels.size();
            uint64_t channel_text_count = 0;
            uint64_t channel_voice_count = 0;
            uint64_t member_count_active = 0;

            uint64_t eventsseen = 0;

            {
                for (auto & bot_ptr : bot.m_clients)
                    eventsseen += bot_ptr->m_sequence;

                for (auto & m : bot.m_members)
                {
                    auto & membr = m.second;
                    if (membr->m_status == aegis::member::member_status::ONLINE)
                        member_online_count++;
                    else if (membr->m_status == aegis::member::member_status::IDLE)
                        member_idle_count++;
                    else if (membr->m_status == aegis::member::member_status::DND)
                        member_dnd_count++;
                }

                for (auto & c : bot.m_channels)
                {
                    auto & chnl = c.second;
                    if (chnl->m_type == channel::ChannelType::TEXT)
                        channel_text_count++;
                    else
                        channel_voice_count++;
                }

                for (auto & g : bot.m_guilds)
                {
                    auto & gld = g.second;
                    member_count += gld->m_members.size();
                }
            }

            std::string members = fmt::format("{} seen\n{} unique\n{} online\n{} idle\n{} dnd", member_count, member_count_unique, member_online_count, member_idle_count, member_dnd_count);
            std::string channels = fmt::format("{} total\n{} text\n{} voice", channel_count, channel_text_count, channel_voice_count);
            std::string guilds = fmt::format("{}", guild_count);
            std::string events = fmt::format("{}", eventsseen);
#if defined(DEBUG) || defined(_DEBUG)
            std::string build_mode = "DEBUG";
#else
            std::string build_mode = "RELEASE";
#endif
            std::string misc = fmt::format("I am shard {} of {} running on `{}` in `{}` mode", shard.m_shardid+1, bot.m_shardidmax, aegis::utility::platform::get_platform(), build_mode);

            fmt::MemoryWriter w;
            w << "[Latest bot source](https://github.com/zeroxs/aegis.cpp)\n[Official Bot Server](https://discord.gg/w7Y3Bb8)\n\nMemory usage: "
                << double(aegis::utility::getCurrentRSS()) / (1024 * 1024) << "MB\nMax Memory: "
                << double(aegis::utility::getPeakRSS()) / (1024 * 1024) << "MB";
            std::string stats = w.str();


            json t = {
                { "title", "AegisBot" },
                { "description", stats },
                { "color", rand() % 0xFFFFFF },
                { "fields",
                json::array(
            {
                { { "name", "Members" },{ "value", members },{ "inline", true } },
                { { "name", "Channels" },{ "value", channels },{ "inline", true } },
                { { "name", "Uptime" },{ "value", bot.uptime() },{ "inline", true } },
                { { "name", "Guilds" },{ "value", guilds },{ "inline", true } },
                { { "name", "Events Seen" },{ "value", events },{ "inline", true } },
                { { "name", u8"\u200b" },{ "value", u8"\u200b" },{ "inline", true } },
                { { "name", "misc" },{ "value", misc },{ "inline", false } }
            }
                    )
                },
                { "footer",{ { "icon_url", "https://cdn.discordapp.com/emojis/289276304564420608.png" },{ "text", "Made in c++ running aegis library" } } }
            };

            _channel.create_message_embed({}, t);
            return true;
        }
        else if (toks[0] == "?source")
        {
            json t = {
                { "title", "AegisBot" },
                { "description", "[Latest bot source](https://github.com/zeroxs/aegis.cpp)\n[Official Bot Server](https://discord.gg/w7Y3Bb8)" },
                { "color", rand() % 0xFFFFFF }
            };
            
            _channel.create_message_embed({}, t);
        }
        else if (toks[0] == "?exit")
        {
//             json obj =
//             {
//                 { "content", "exiting..." }
//             };
//             bot.get_channel(channel_id).create_message(obj.dump());
//             bot.set_state(SHUTDOWN);
//             bot.websocket().close(shard.m_connection, 1001, "");
//             bot.stop_work();
            return true;
        }
        else if (toks[0] == "?nuke")
        {
            //bot.websocket().close(shard.m_connection, 1001, "");
            return true;
        }
        else if (toks[0] == "?test")
        {
            //_channel.create_message("test message");
            return true;
        }
        else if (toks[0] == "?shard")
        {
            _channel.create_message(fmt::format("I am shard#[{}]", shard.m_shardid));
            return true;
        }
        else if (toks[0] == "?shards")
        {
            std::string s = "```\n";
            for (auto & shard : bot.m_clients)
            {
                s += fmt::format("shard#{} tracking {:4} guilds {:4} channels {:4} members {:4} messages {:4} presence updates\n", shard->m_shardid, shard->counters.guilds, shard->counters.channels, shard->counters.members, shard->counters.messages, shard->counters.presence_changes);
            }
            s += "```";
            _channel.create_message(s);
        }
        else if (toks[0] == "?createchannel")
        {
            if (!_guild.create_text_channel(toks[1], 0, false, {}))
            {
                _channel.create_message("No perms CREATE_CHANNEL");
            }
        }
        else if (toks[0] == "?deletechannel")
        {
            if (userid == 171000788183678976)
            {
                if (!_channel.delete_channel())
                {
                    _channel.create_message("No perms DELETE_CHANNEL");
                }
            }
        }
        else if (toks[0] == "?react")
        {
            //_channel.create_reaction(std::stoll(toks[1]), toks[2]);
        }
        else if (toks[0] == "?deletereact")
        {
            //_channel.delete_own_reaction(std::stoll(toks[1]), toks[2]);
        }
        else if (toks[0] == "?options")
        {
            //_channel.create_reaction(std::stoll(toks[1]), "e:289276304564420608");
            //_channel.create_reaction(std::stoll(toks[1]), "e:367566538427072523");
            //_channel.create_reaction(std::stoll(toks[1]), "e:288902947046424576");
        }
        else if (toks[0] == "?remoptions")
        {
            //_channel.delete_own_reaction(std::stoll(toks[1]), "e:289276304564420608");
            //_channel.delete_own_reaction(std::stoll(toks[1]), "e:367566538427072523");
            //_channel.delete_own_reaction(std::stoll(toks[1]), "e:288902947046424576");
        }
        else if (toks[0] == "?serverlist")
        {
            std::stringstream w;
            for (auto & g : bot.m_guilds)
            {
                auto & gld = g.second;
                w << "*" << gld->m_name << "*  :  " << gld->m_id << "\n";
            }


            json t = {
                { "title", "Server List" },
                { "description", w.str() },
                { "color", 10599460 }
            };
            //_channel.create_message_embed("", t);
        }
        else if (toks[0] == "?roles")
        {
            std::stringstream w;
            for (auto & r : _guild.m_roles)
            {
                w << "[" << r.second->id << "] : [A:" << r.second->_permission.getAllowPerms() << "] : [D:" << r.second->_permission.getDenyPerms() << "] : [" << r.second->name << "]\n";
            }
            //_channel.create_message(w.str());
        }
        else if (toks[0] == "?mroles")
        {
            std::stringstream w;
            auto & gld = _guild.m_members[std::stoll(toks[1])]->m_guilds[_guild.m_id];
            for (auto & rl : gld.roles)
            {
                role & r = _guild.get_role(rl);
                w << "[" << r.id << "] : [A:" << r._permission.getAllowPerms() << "] : [D:" << r._permission.getDenyPerms() << "] : [" << r.name << "]\n";

            }
            //_channel.create_message(w.str());
        }
        else if (toks[0] == "?croles")
        {
            std::stringstream w;
            for (auto & r : _channel.m_overrides)
            {
                auto & a = r.second;
                std::string name;
                if (a.type == perm_overwrite::ORType::USER)
                    name = _guild.m_members[a.id]->getFullName();
                else
                    name = _guild.m_roles[a.id]->name;
                w << "[" << ((a.type== perm_overwrite::ORType::USER)?"user":"role") << "] : [A:" << a.allow << "] : [D:" << a.deny << "] : [" << name << "]\n";
            }
            //_channel.create_message(w.str());
        }

        return true;
    }

    bool message_update(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool message_delete(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool message_delete_bulk(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool guild_create(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool guild_update(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool guild_delete(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool user_settings_update(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool user_update(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool ready(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool resumed(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool channel_create(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool channel_update(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool channel_delete(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool guild_ban_add(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool guild_ban_remove(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool guild_emojis_update(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool guild_integrations_update(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool guild_member_add(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool guild_member_remove(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool guild_member_update(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool guild_member_chunk(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool guild_role_create(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool guild_role_update(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool guild_role_delete(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool presence_update(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool voice_state_update(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }

    bool voice_server_update(json & msg, client & shard, Aegis & bot)
    {
        return true;
    }
};

}

