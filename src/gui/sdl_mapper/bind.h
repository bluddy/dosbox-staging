#ifndef DOSBOX_MAPPER_BIND
#define DOSBOX_MAPPER_BIND

#include <list>
#include <vector>
#include <string>
#include <memory>

#include "SDL.h"
#include "types.h"

typedef std::list<std::shared_ptr<CBind>> CBindList;

class CEvent;

class CBind {
public:
	CBind() {}

	virtual ~CBind() {}

	CBind(const CBind&) = delete; // prevent copy
	CBind& operator=(const CBind&) = delete; // prevent assignment

	std::string GetFlagsStr() const;
	void SetFlagsFromStr(std::string flag_s);

    // use value-boundary for on/off events
	void ActivateBind(Bits _value, bool ev_trigger, bool skip_action=false);
	void DeActivateBind(bool ev_trigger);
	virtual std::string GetConfigName() const = 0;
	virtual std::string GetBindName() const = 0;

	Bitu mods{0};
	Bitu flags{0};

    // Event we belong to
	CEvent *event{nullptr};
	bool active{false};
	bool holding{false};
};


class CKeyBind final : public CBind {
public:
	CKeyBind(CBindList *_list, SDL_Scancode _key)
		: CBind(_list),
		  key(_key)
	{}

	std::string GetBindName() const override;

	std::string ConfigName() const override
	{
        std::ostringstream oss;
        oss << "key " << key;
        return oss.str();
	}

public:
	SDL_Scancode key;
};

class CJAxisBind;
class CJButtonBind;
class CJHatBind;

class CJAxisBind final : public CBind {
public:
	CJAxisBind(CBindList *_list, CBindGroup *_group, int _axis, bool _positive)
		: CBind(_list),
		  group(_group),
		  axis(_axis),
		  positive(_positive)
	{}

	CJAxisBind(const CJAxisBind&) = delete; // prevent copy
	CJAxisBind& operator=(const CJAxisBind&) = delete; // prevent assignment

	void ConfigName(char *buf) override
	{
		sprintf(buf, "%s axis %d %d",
		        group->ConfigStart(),
		        axis,
		        positive ? 1 : 0);
	}

	std::string GetBindName() const override
	{
		char buf[30];
		safe_sprintf(buf, "%s Axis %d%s", group->BindStart(), axis,
		             positive ? "+" : "-");
		return buf;
	}

protected:
	CBindGroup *group;
	int axis;
	bool positive;
};

class CJButtonBind final : public CBind {
public:
	CJButtonBind(CBindList *_list, CBindGroup *_group, int _button)
		: CBind(_list),
		  group(_group),
		  button(_button)
	{}

	CJButtonBind(const CJButtonBind&) = delete; // prevent copy
	CJButtonBind& operator=(const CJButtonBind&) = delete; // prevent assignment

	void ConfigName(char *buf) override
	{
		sprintf(buf, "%s button %d", group->ConfigStart(), button);
	}

	std::string GetBindName() const override
	{
		char buf[30];
		safe_sprintf(buf, "%s Button %d", group->BindStart(), button);
		return buf;
	}

protected:
	CBindGroup *group;
	int button;
};

class CJHatBind final : public CBind {
public:
	CJHatBind(CBindList *_list, CBindGroup *_group, uint8_t _hat, uint8_t _dir);

	CJHatBind(const CJHatBind&) = delete; // prevent copy
	CJHatBind& operator=(const CJHatBind&) = delete; // prevent assignment

	void ConfigName(char *buf) override;
	std::string GetBindName() const override;

protected:
	CBindGroup *group;
	uint8_t hat;
	uint8_t dir;
};

#endif