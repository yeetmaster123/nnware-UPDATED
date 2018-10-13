struct message_t
{
	explicit message_t(std::string msg) : m_message(std::move(msg)), m_start(gpGlobals->curtime), m_alpha(1.f), m_width(render::text_size(fonts::main, m_message.c_str()).x) { }

	std::string m_message;
	float m_start;
	float m_alpha;
	float m_width;
};