
struct KeyBlock {
	const char * title;
	const char * entry;
	KBD_KEYS key;
};
static KeyBlock combo_f[12]={
	{"F1","f1",KBD_f1},		{"F2","f2",KBD_f2},		{"F3","f3",KBD_f3},
	{"F4","f4",KBD_f4},		{"F5","f5",KBD_f5},		{"F6","f6",KBD_f6},
	{"F7","f7",KBD_f7},		{"F8","f8",KBD_f8},		{"F9","f9",KBD_f9},
	{"F10","f10",KBD_f10},	{"F11","f11",KBD_f11},	{"F12","f12",KBD_f12},
};

static KeyBlock combo_1[14]={
	{"`~","grave",KBD_grave},	{"1!","1",KBD_1},	{"2@","2",KBD_2},
	{"3#","3",KBD_3},			{"4$","4",KBD_4},	{"5%","5",KBD_5},
	{"6^","6",KBD_6},			{"7&","7",KBD_7},	{"8*","8",KBD_8},
	{"9(","9",KBD_9},			{"0)","0",KBD_0},	{"-_","minus",KBD_minus},
	{"=+","equals",KBD_equals},	{"\x1B","bspace",KBD_backspace},
};

static KeyBlock combo_2[12] = {
        {"Q", "q", KBD_q},
        {"W", "w", KBD_w},
        {"E", "e", KBD_e},
        {"R", "r", KBD_r},
        {"T", "t", KBD_t},
        {"Y", "y", KBD_y},
        {"U", "u", KBD_u},
        {"I", "i", KBD_i},
        {"O", "o", KBD_o},
        {"P", "p", KBD_p},
        {"[{", "lbracket", KBD_leftbracket},
        {"]}", "rbracket", KBD_rightbracket},
};

static KeyBlock combo_3[12]={
	{"A","a",KBD_a},			{"S","s",KBD_s},	{"D","d",KBD_d},
	{"F","f",KBD_f},			{"G","g",KBD_g},	{"H","h",KBD_h},
	{"J","j",KBD_j},			{"K","k",KBD_k},	{"L","l",KBD_l},
	{";:","semicolon",KBD_semicolon},				{"'\"","quote",KBD_quote},
	{"\\|","backslash",KBD_backslash},
};

static KeyBlock combo_4[12] = {{"\\|", "oem102", KBD_oem102},
                               {"Z", "z", KBD_z},
                               {"X", "x", KBD_x},
                               {"C", "c", KBD_c},
                               {"V", "v", KBD_v},
                               {"B", "b", KBD_b},
                               {"N", "n", KBD_n},
                               {"M", "m", KBD_m},
                               {",<", "comma", KBD_comma},
                               {".>", "period", KBD_period},
                               {"/?", "slash", KBD_slash},
                               {"/?", "abnt1", KBD_abnt1}};
