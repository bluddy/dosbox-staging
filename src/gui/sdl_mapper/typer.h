#ifndef SDL_MAPPER_TYPER_H
#define SDL_MAPPER_TYPER_H

class Typer {
public:
	Typer() = default;
	Typer(const Typer &) = delete;            // prevent copy
	Typer &operator=(const Typer &) = delete; // prevent assignment
	~Typer() { Stop(); }
	void Start(std::vector<std::unique_ptr<CEvent>> *ext_events,
	           std::vector<std::string> &ext_sequence,
	           const uint32_t wait_ms,
	           const uint32_t pace_ms)
	{
		// Guard against empty inputs
		if (!ext_events || ext_sequence.empty())
			return;
		Wait();
		m_events = ext_events;
		m_sequence = std::move(ext_sequence);
		m_wait_ms = wait_ms;
		m_pace_ms = pace_ms;
		m_stop_requested = false;
		m_instance = std::thread(&Typer::Callback, this);
		set_thread_name(m_instance, "dosbox:autotype");
	}
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
	CEvent *GetLShiftEvent()
	{
		static CEvent *lshift_event = nullptr;
		for (auto &event : *m_events) {
			if (std::string("key_lshift") == event->GetName()) {
				lshift_event = event.get();
				break;
			}
		}
		assert(lshift_event);
		return lshift_event;
	}

	void Callback()
	{
		// quit before our initial wait time
		if (m_stop_requested)
			return;
		std::this_thread::sleep_for(std::chrono::milliseconds(m_wait_ms));
		for (const auto &button : m_sequence) {
			if (m_stop_requested)
				return;
			bool found = false;
			// comma adds an extra pause, similar to on phones
			if (button == ",") {
				found = true;
				// quit before the pause
				if (m_stop_requested)
					return;
				std::this_thread::sleep_for(std::chrono::milliseconds(m_pace_ms));
				// Otherwise trigger the matching button if we have one
			} else {
				// is the button an upper case letter?
				const auto is_cap = button.length() == 1 && isupper(button[0]);
				const auto maybe_lshift = is_cap ? GetLShiftEvent() : nullptr;
				const std::string lbutton = is_cap ? std::string{int_to_char(
				                                             tolower(button[0]))}
				                                   : button;
				const std::string bind_name = "key_" + lbutton;
				for (auto &event : *m_events) {
					if (bind_name == event->GetName()) {
						found = true;
						if (maybe_lshift)
							maybe_lshift->Active(true);
						event->Active(true);
						std::this_thread::sleep_for(
						        std::chrono::milliseconds(50));
						event->Active(false);
						if (maybe_lshift)
							maybe_lshift->Active(false);
						break;
					}
				}
			}
			/*
			 *  Terminate the sequence for safety reasons if we can't find
			 * a button. For example, we don't wan't DEAL becoming DEL, or
			 * 'rem' becoming 'rm'
			 */
			if (!found) {
				LOG_MSG("MAPPER: Couldn't find a button named '%s', stopping.",
				        button.c_str());
				return;
			}
			if (m_stop_requested) // quit before the pacing delay
				return;
			std::this_thread::sleep_for(std::chrono::milliseconds(m_pace_ms));
		}
	}

	std::thread m_instance = {};
	std::vector<std::string> m_sequence = {};
	std::vector<std::unique_ptr<CEvent>>* m_events = nullptr;
	uint32_t m_wait_ms = 0;
	uint32_t m_pace_ms = 0;
	std::atomic_bool m_stop_requested{false};
};


#endif