#ifndef SDL_MAPPER_TYPER_H
#define SDL_MAPPER_TYPER_H

#include <thread>
#include <vector>
#include <string>
#include <atomic>

class CEvent;

class Typer {
public:
	Typer() = default;
	Typer(const Typer &) = delete;            // prevent copy
	Typer &operator=(const Typer &) = delete; // prevent assignment
	~Typer() { Stop(); }
	void Start(std::vector<std::unique_ptr<CEvent>> *ext_events,
	           std::vector<std::string> &ext_sequence,
	           const uint32_t wait_ms,
	           const uint32_t pace_ms);
	void Wait()
	{
		if (m_instance.joinable())
			m_instance.join();
	}
	void Stop()
	{
		m_stop_requested = true;
		Wait();
	}
	void StopImmediately()
	{
		m_stop_requested = true;
		if (m_instance.joinable())
			m_instance.detach();
	}

private:
	// find the event for the lshift key and return it
	CEvent *GetLShiftEvent();

	void Callback();

	std::thread m_instance = {};
	std::vector<std::string> m_sequence = {};
	std::vector<std::unique_ptr<CEvent>>* m_events = nullptr;
	uint32_t m_wait_ms = 0;
	uint32_t m_pace_ms = 0;
	std::atomic_bool m_stop_requested{false};
};


#endif