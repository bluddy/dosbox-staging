#include "edit_binding_ui.h"

void EditBindingUI::ChangeActionText(const char* text, const Rgb888& col)
{
	bind_but.action->Change(text,"");
	bind_but.action->SetColor(col);
}

std::string EditBindingUI::HumanizeKeyName(const CBindList &binds, const std::string &fallback)
{
	auto trim_prefix = [](const std::string& bind_name) {
		if (starts_with(bind_name, "Left ")) {
			return bind_name.substr(sizeof("Left"));
		}
		if (starts_with(bind_name, "Right ")) {
			return bind_name.substr(sizeof("Right"));
		}
		return bind_name;
	};

	const auto binds_num = binds.size();

	// We have a single bind, just use it
	if (binds_num == 1)
		return binds.front()->GetBindName();

	// Avoid prefix, e.g. "Left Alt" and "Right Alt" -> "Alt"
	if (binds_num == 2) {
		const auto key_name_1 = trim_prefix(binds.front()->GetBindName());
		const auto key_name_2 = trim_prefix(binds.back()->GetBindName());
		if (key_name_1 == key_name_2) {
			if (fallback.empty())
				return key_name_1;
			else
				return fallback + ": " + key_name_1;
		}
	}

	return fallback;
}

void EditBindingUI::Update()
{
	if (mapper.abind == nullptr) {
		bind_but.bind_title->Enable(false);
		bind_but.del->Enable(false);
		bind_but.next->Enable(false);
		bind_but.mod1->Enable(false);
		bind_but.mod2->Enable(false);
		bind_but.mod3->Enable(false);
		bind_but.hold->Enable(false);
		return;
	}

	// Count number of bindings for active event and the number (pos)
	// of active bind
	size_t active_event_binds_num = 0;
	size_t active_bind_pos = 0;
	if (mapper.aevent) {
		const auto &bindlist = mapper.aevent->bindlist;
		active_event_binds_num = bindlist.size();
		for (const auto *bind : bindlist) {
			if (bind == mapper.abind)
				break;
			active_bind_pos += 1;
		}
	}

	std::string mod_1_desc = "";
	std::string mod_2_desc = "";
	std::string mod_3_desc = "";

	// Correlate mod event bindlists to button labels and prepare
	// human-readable mod key names.
	for (auto &event : events) {
		using namespace std::string_literals;

		assert(event);
		const auto bindlist = event->bindlist;

		if (event->GetName() == "mod_1"s) {
			bind_but.mod1->Enable(!bindlist.empty());
			bind_but.mod1->SetText(humanize_key_name(bindlist, "Mod1"));
			const std::string txt = humanize_key_name(bindlist, "");
			mod_1_desc = (txt.empty() ? "Mod1" : txt) + " + ";
			continue;
		}
		if (event->GetName() == "mod_2"s) {
			bind_but.mod2->Enable(!bindlist.empty());
			bind_but.mod2->SetText(humanize_key_name(bindlist, "Mod2"));
			const std::string txt = humanize_key_name(bindlist, "");
			mod_2_desc = (txt.empty() ? "Mod1" : txt) + " + ";
			continue;
		}
		if (event->GetName() == "mod_3"s) {
			bind_but.mod3->Enable(!bindlist.empty());
			bind_but.mod3->SetText(humanize_key_name(bindlist, "Mod3"));
			const std::string txt = humanize_key_name(bindlist, "");
			mod_3_desc = (txt.empty() ? "Mod1" : txt) + " + ";
			continue;
		}
	}

	// Format "Bind: " description
	const auto mods = mapper.abind->mods;
	bind_but.bind_title->Change("Bind %" PRIuPTR "/%" PRIuPTR ": %s%s%s%s",
	                            active_bind_pos + 1, active_event_binds_num,
	                            (mods & BMOD_Mod1 ? mod_1_desc.c_str() : ""),
	                            (mods & BMOD_Mod2 ? mod_2_desc.c_str() : ""),
	                            (mods & BMOD_Mod3 ? mod_3_desc.c_str() : ""),
	                            mapper.abind->GetBindName().c_str());

	bind_but.bind_title->SetColor(color_green);
	bind_but.bind_title->Enable(true);
	bind_but.del->Enable(true);
	bind_but.next->Enable(active_event_binds_num > 1);
	bind_but.hold->Enable(true);
}

void EditBindingUI::SetActiveBind(CBind *new_active_bind)
{
	mapper.abind = new_active_bind;
	update_active_bind_ui();
}

void EditBindingUI::SetActiveEvent(CEvent * event)
{
	mapper.aevent=event;
	mapper.redraw=true;
	mapper.addbind=false;
	bind_but.event_title->Change("   Event: %s", event ? event->GetName() : "none");
	if (!event) {
		change_action_text("Select an event to change.", color_white);
		bind_but.add->Enable(false);
		SetActiveBind(nullptr);
	} else {
		change_action_text("Modify the bindings for this event or select a different event.",
		                   color_white);
		mapper.abindit=event->bindlist.begin();
		if (mapper.abindit!=event->bindlist.end()) {
			SetActiveBind(*(mapper.abindit));
		} else SetActiveBind(nullptr);
		bind_but.add->Enable(true);
	}
}
