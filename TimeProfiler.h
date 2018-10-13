#pragma once
#include "includes.h"

namespace UTILS
{
	class TimeProfiler
	{
	public:
		void Initialize(std::string profiler_name, double record_session_time = 1)
		{
			m_profiler_name = profiler_name;
			m_is_recording = false;
			m_record_session_time = record_session_time;
			Reset();
		}

		void Start()
		{
			m_is_recording = true;
			m_start_time = std::chrono::high_resolution_clock::now();
		}

		void End()
		{
			m_total_elapsed_time += std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - m_start_time).count();
			m_is_recording = false;

			if (std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - m_initialization_time).count() > m_record_session_time)
				Reset();
		}

	private:
		std::chrono::high_resolution_clock::time_point m_initialization_time;
		std::chrono::high_resolution_clock::time_point m_start_time;

		double m_total_elapsed_time;
		double m_record_session_time;

		bool m_is_recording;
		std::string m_profiler_name;

		void Reset()
		{
			std::cout << m_profiler_name << ": " << m_total_elapsed_time << std::endl;

			m_total_elapsed_time = 0;
			m_initialization_time = std::chrono::high_resolution_clock::now();
		}
	};
}
