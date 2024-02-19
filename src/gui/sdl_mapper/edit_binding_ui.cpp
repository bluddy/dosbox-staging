
void EditBindingUI::ChangeActionText(const char* text, const Rgb888& col)
{
	bind_but.action->Change(text,"");
	bind_but.action->SetColor(col);
}
