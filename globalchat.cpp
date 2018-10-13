#include "globalchat.h"
/*
* Call when receiving a new message from the server.
* This can be done in a lot of ways although I would recommend
* looking into this post: https://www.unknowncheats.me/forum/counterstrike-global-offensive/223748-dispatchusermessage-reader.html
* You could also just listen to player_say(?) event but this
* wont be called on Valve servers afaik
*/
void world_chat::run_message(const int i, std::string& msg)
{
	m_chatter.at(i).emplace_back(msg);
}

/*
* Call whereever you render your shit
*/
void world_chat::run_render()
{
	if (!gpEngine->IsInGame() || !gpEngine->IsConnected())
		return;

	if (!g.local)
		return;

	/*
	* Meaning that it will reach 1 within 2.5seconds
	*/
	constexpr auto fade_frequency = 1.f / 1.f;

	for (auto i = 0; i < gpGlobals->maxClients; i++)
	{
		auto player = CBasePlayer::get(i);
		if (!player->valid())
			continue;

		/*
		* Change this if you want this to be shown only on enemies
		*/
		/*if ( player->same_team( g.local ) )
		continue; */

		auto& chat = m_chatter.at(i);

		/*
		* No messages stored
		*/
		if (chat.empty())
			continue;

		/*
		* We will draw messages just above their head
		*/
		const auto top_world = player->hitbox_pos(HITBOX_HEAD) + Vector(0, 0, /*random value*/18);

		/*
		* Our screen pos
		*/
		auto top = Vector();

		/*
		* Check if we can get a valid screen position.
		* If we can't -> go onto the next player
		*/
		if (!math::world_to_screen(top_world, top))
			continue;

		/*
		* Draw the box chat elements
		*/
		const auto width = get_biggest_width(i) + /*padding*/4 * 2;
		const auto height = chat.size() * render::font_size(fonts::main).y;

		render::outlined_filled_rect({ top.x - width / 2, top.y - height }, { width, height }, Color(70, 70, 70), Color(5, 5, 5));

		for (auto j = 0; j < chat.size(); j++)
		{
			auto& cur = chat.at(j);

			/*
			* Render chat message
			*/
			render::text(fonts::main, { top.x, top.y - (j + 1) * render::font_size(fonts::main).y }, Color::White(cur.m_alpha * 255.f), FONT_CENTER_X, cur.m_message.c_str());

			/*
			* Message has been fully visible for over 4 seconds, fade it out now
			*/
			if (gpGlobals->curtime - cur.m_start > 4.f)
				cur.m_alpha -= fade_frequency * gpGlobals->frametime;

			/*
			* Message expired
			*/
			if (cur.m_alpha <= 0.f)
				chat.erase(chat.begin() + j);
		}

	}
}

float world_chat::get_biggest_width(const int i)
{
	auto ret = -1.f;

	const auto chat = m_chatter.at(i);

	for (auto msg : chat)
	{
		if (msg.m_width > ret)
			ret = msg.m_width;
	}

	return ret;
}

std::vector<message_t> world_chat::get_messages(const int i)
{
	return m_chatter.at(i);
}