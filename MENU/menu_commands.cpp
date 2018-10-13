#include "../includes.h"
#include "../UTILS/render.h"

#include "menu_commands.h"


namespace MENU
{
	namespace PPGUI_PP_GUI
	{
		WindowCommand::WindowCommand(std::string Name, Vector2D Position, Vector2D Size, int Scroll_Size)
		{
			if (menu.GetFont() == -1)
				throw std::runtime_error("No font provided");

			info.object_type = OBJECT_TYPE::OBJECT_WINDOW;
			info.font = menu.GetFont();
			info.name = Name;
			info.id_name = CreateIDName();
			info.name = Menu::FilterName(Name);
			info.position = Position + menu.GetCommandData(GetID()).window_data.offset;
			info.size = Size;

			scroll_size = Scroll_Size;
		}
		void WindowCommand::Draw()
		{
			auto position = info.position;
			auto size = info.size;

			// draws the window body
			RENDER::DrawFilledRectOutline(position.x, position.y +
				WINDOW_TITLE_BAR_SIZE, position.x + size.x, position.y
				+ info.size.y, colors[WINDOW_BODY]);

			// draws the window title bar
			RENDER::DrawFilledRectOutline(position.x, position.y,
				position.x + size.x, position.y + WINDOW_TITLE_BAR_SIZE,
				colors[WINDOW_TITLE_BAR]);

			// draw the name
			auto center = position + (size * 0.5f);
			RENDER::DrawF(center.x, position.y + (WINDOW_TITLE_BAR_SIZE * 0.5f),
				info.font, true, true, colors[WINDOW_TEXT], info.name);

			/* SCROLLBAR */
			if (!scroll_size)
				return;

			auto scrollbar_position = Vector2D((position.x + size.x) - (OBJECT_SCROLLBAR_WIDTH + OBJECT_PADDING + OBJECT_PADDING), position.y + WINDOW_TITLE_BAR_SIZE + FRAME_PADDING);
			auto scrollbar_size = Vector2D(OBJECT_SCROLLBAR_WIDTH, size.y - (FRAME_PADDING + FRAME_PADDING + WINDOW_TITLE_BAR_SIZE));
			int scrollbar_fraction = OBJECT_SCROLLBAR_FRACTION * scrollbar_size.y;

			int scrollbar_offset = (-menu.GetCommandData(GetID()).window_data.scroll_offset) * ((scrollbar_size.y - scrollbar_fraction) / scroll_size);
			scrollbar_offset = UTILS::clamp<int>(scrollbar_offset, 0, scrollbar_size.y - scrollbar_fraction);

			// draw the scrollbar
			RENDER::DrawFilledRectOutline(scrollbar_position.x, scrollbar_position.y + 
				scrollbar_offset, scrollbar_position.x + scrollbar_size.x, 
				scrollbar_position.y + scrollbar_fraction + scrollbar_offset, 
				colors[SCROLLBAR_BODY]);
		}
		void WindowCommand::ModifyScrollSize(int new_scroll)
		{
			scroll_size = max(scroll_size, new_scroll);
		}
		void WindowCommand::TestInput(Input& input)
		{
			// dragging
			if (input.mouse.left_button & 1 && UTILS::is_point_in_range(
				input.mouse.position, info.position, Vector2D(info.size.x,
				WINDOW_TITLE_BAR_SIZE)))
			{
				menu.GetCommandData(GetID()).window_data.is_dragging = true;
				input.mouse.left_button = 0;
			}
			else if (!input.real_mouse.left_button)
				menu.GetCommandData(GetID()).window_data.is_dragging = false;

			// add to the window offset when dragging
			if (menu.GetCommandData(GetID()).window_data.is_dragging)
				menu.GetCommandData(GetID()).window_data.offset += input.mouse.position_delta;

			if (!UTILS::is_point_in_range(input.mouse.position, info.position, info.size))
				return;

			menu.GetCommandData(GetID()).window_data.scroll_offset = UTILS::clamp<int>(
				menu.GetCommandData(GetID()).window_data.scroll_offset, -scroll_size, 0);

			if (!scroll_size)
				return;

			auto scrollbar_position = Vector2D((info.position.x + info.size.x) - (OBJECT_SCROLLBAR_WIDTH + OBJECT_PADDING + OBJECT_PADDING), info.position.y + WINDOW_TITLE_BAR_SIZE + FRAME_PADDING);
			auto scrollbar_size = Vector2D(OBJECT_SCROLLBAR_WIDTH, info.size.y - (FRAME_PADDING + FRAME_PADDING + WINDOW_TITLE_BAR_SIZE));
			int scrollbar_fraction = OBJECT_SCROLLBAR_FRACTION * scrollbar_size.y;

			int scrollbar_offset = (-menu.GetCommandData(GetID()).window_data.scroll_offset) * ((scrollbar_size.y - scrollbar_fraction) / scroll_size);
			scrollbar_offset = UTILS::clamp<int>(scrollbar_offset, 0, scrollbar_size.y - scrollbar_fraction);

			auto new_scrollbar_position = scrollbar_position + Vector2D(0, scrollbar_offset);
			auto new_scrollbar_size = Vector2D(scrollbar_size.x, scrollbar_fraction);

			// if dragging scroll bar
			if (UTILS::is_point_in_range(input.mouse.position, new_scrollbar_position, new_scrollbar_size) && input.mouse.left_button & 1)
			{
				menu.GetCommandData(GetID()).window_data.is_scrolling = true;
				input.mouse.left_button = 0;
			}
			else if (input.real_mouse.left_button & 1)
				menu.GetCommandData(GetID()).window_data.is_scrolling = false;

			if (menu.GetCommandData(GetID()).window_data.is_scrolling && input.mouse.left_button)
			{
				input.mouse.left_button = 0;
				menu.GetCommandData(GetID()).window_data.scroll_offset -= input.mouse.position_delta.y * (scroll_size / (scrollbar_size.y - scrollbar_fraction));
			}


			// scrolling
			if (input.mouse.scroll)
				menu.GetCommandData(GetID()).window_data.scroll_offset += 
					input.mouse.scroll > 0 ? OBJECT_SCROLL_SPEED : -OBJECT_SCROLL_SPEED;

			// clamp scroll offset
			menu.GetCommandData(GetID()).window_data.scroll_offset = UTILS::clamp<int>(
				menu.GetCommandData(GetID()).window_data.scroll_offset, -scroll_size, 0);
		}
		CommandInfo WindowCommand::Info()
		{
			return info;
		}

		GroupboxCommand::GroupboxCommand(std::string Name, Vector2D Position, Vector2D Size, FrameBounds Bounds, int Scroll_Size)
		{
			if (menu.GetFont() == -1)
				throw std::runtime_error("No font provided");

			info.position = Position;
			info.size = Size;
			info.object_type = OBJECT_TYPE::OBJECT_GROUPBOX;
			info.font = menu.GetFont();
			info.name = Name;
			info.id_name = CreateIDName();
			info.name = Menu::FilterName(Name);
			frame_bounds = Bounds;
			scroll_size = Scroll_Size;
		}
		void GroupboxCommand::Draw()
		{
			bool is_top_in_bounds = UTILS::is_point_in_range(info.position,
				frame_bounds.min, frame_bounds.max - frame_bounds.min);
			bool is_bottom_in_bounds = UTILS::is_point_in_range(
				info.position + info.size, frame_bounds.min, frame_bounds.max - 
				frame_bounds.min);

			if (!(is_top_in_bounds || is_bottom_in_bounds))
				return;

			auto position = info.position;
			auto size = info.size;

			if (!is_top_in_bounds)
			{
				position.y = UTILS::clamp<int>(position.y, frame_bounds.min.y, position.y + size.y);
				size.y -= position.y - info.position.y;
			}
			if (!is_bottom_in_bounds)
				size.y = frame_bounds.max.y - position.y;

			// draws the groupbox body
			RENDER::DrawFilledRect(position.x, position.y,
				position.x + size.x, position.y + size.y,
				colors[GROUPBOX_BODY]);

			if (is_top_in_bounds)
			{
				// if its an unnamed groupbox
				if (!info.name.size())
					RENDER::DrawLine(position.x, position.y, position.x + size.x, position.y, colors[GROUPBOX_OUTLINE]);
				else
				{
					// draw the top outline so that it doesn't touch the text
					RENDER::DrawLine(position.x, position.y, position.x
						+ 16, position.y, colors[GROUPBOX_OUTLINE]);
					RENDER::DrawLine(position.x + 24 + RENDER::GetTextSize(info.font,
						info.name).x, position.y, position.x + size.x,
						position.y, colors[GROUPBOX_OUTLINE]);

					RENDER::DrawF(position.x + 20, position.y,
						info.font, false, true, colors[GROUPBOX_TEXT], info.name);
				}
			}

			// draw the left and right outline
			RENDER::DrawEmptyRect(position.x, position.y, position.x + size.x, 
				position.y + size.y, colors[GROUPBOX_OUTLINE], 0b1 | 0b100);

			if (is_bottom_in_bounds)
			{
				RENDER::DrawLine(position.x + size.x, position.y +
					size.y, position.x, position.y + size.y,
					colors[GROUPBOX_OUTLINE]);
			}
		}
		void GroupboxCommand::ModifyScrollSize(int new_scroll)
		{
			scroll_size = max(scroll_size, new_scroll);
		}
		void GroupboxCommand::TestInput(Input& input)
		{
			if (!UTILS::is_point_in_range(input.mouse.position, info.position, info.size))
				return;

			// clamp scrolling even if we aren't scrolling since it might get changed with ModifyScrollSize()
			menu.GetCommandData(GetID()).window_data.scroll_offset = UTILS::clamp<int>(
				menu.GetCommandData(GetID()).window_data.scroll_offset, -scroll_size, 0);
			if (!input.mouse.scroll)
				return;

			// scrolling
			menu.GetCommandData(GetID()).window_data.scroll_offset += 
				input.mouse.scroll > 0 ? OBJECT_SCROLL_SPEED : -OBJECT_SCROLL_SPEED;
			menu.GetCommandData(GetID()).window_data.scroll_offset = UTILS::clamp<int>(
				menu.GetCommandData(GetID()).window_data.scroll_offset, -scroll_size, 0);

			input.mouse.scroll = 0;
		}
		
		CommandInfo GroupboxCommand::Info()
		{
			return info;
		}

		SeparatorCommand::SeparatorCommand(std::string name, Vector2D position, Vector2D size, FrameBounds bounds)
		{
			if (menu.GetFont() == -1)
				throw std::runtime_error("No font provided");

			info.position = position;
			info.size = size;
			info.object_type = OBJECT_TYPE::OBJECT_SEPARATOR;
			info.font = menu.GetFont();
			info.name = name;
			info.id_name = CreateIDName();
			info.name = Menu::FilterName(name);
			frame_bounds = bounds;
		}
		void SeparatorCommand::Draw()
		{
			auto text_size = RENDER::GetTextSize(info.font, info.name);
			auto size = info.size;
			size.y = text_size.y;
			auto position = info.position;
			auto center = position + (size * 0.5f);

			// make sure in bounds
			if (!UTILS::is_point_in_range(info.position, frame_bounds.min, frame_bounds.max - frame_bounds.min) || 
				!UTILS::is_point_in_range(info.position + size, frame_bounds.min, frame_bounds.max - frame_bounds.min))
				return;

			RENDER::DrawF(position.x + SEPARATOR_TEXT_OFFSET, position.y + (size.y * 0.5f), info.font, false, true, colors[SEPARATOR_TEXT], info.name);

			RENDER::DrawLine(position.x, center.y, position.x + (SEPARATOR_TEXT_OFFSET - 5), center.y, colors[SEPARATOR_LINE]);
			RENDER::DrawLine(position.x + SEPARATOR_TEXT_OFFSET + 5 + text_size.x, center.y, position.x + size.x, center.y, colors[SEPARATOR_LINE]);
		}
		CommandInfo SeparatorCommand::Info()
		{
			return info;
		}

		CheckboxCommand::CheckboxCommand(std::string Name, Vector2D Position, Vector2D Size, FrameBounds bounds, bool& variable)
		{
			if (menu.GetFont() == -1)
				throw std::runtime_error("No font provided");

			value = &variable;

			info.position = Position;
			info.size = Size;
			info.object_type = OBJECT_TYPE::OBJECT_CHECKBOX;
			info.font = menu.GetFont();
			info.name = Name;
			info.id_name = CreateIDName();
			info.name = Menu::FilterName(Name);

			frame_bounds = bounds;

			menu.GetCommandData(GetID()).checkbox_value = variable;
		}
		CheckboxCommand::CheckboxCommand(std::string Name, Vector2D Position, Vector2D Size, FrameBounds bounds)
		{
			if (menu.GetFont() == -1)
				throw std::runtime_error("No font provided");

			info.position = Position;
			info.size = Size;
			info.object_type = OBJECT_TYPE::OBJECT_CHECKBOX;
			info.font = menu.GetFont();
			info.name = Name;
			info.id_name = CreateIDName();
			info.name = Menu::FilterName(Name);

			frame_bounds = bounds;
		}
		void CheckboxCommand::Draw()
		{
			bool is_top_in_bounds = UTILS::is_point_in_range(info.position,
				frame_bounds.min, frame_bounds.max - frame_bounds.min);
			bool is_bottom_in_bounds = UTILS::is_point_in_range(
				info.position + info.size, frame_bounds.min, frame_bounds.max -
				frame_bounds.min);

			// dont draw if not in window
			if (!(is_top_in_bounds || is_bottom_in_bounds))
				return;

			auto position = info.position;
			auto size = info.size;
			if (!is_top_in_bounds)
			{
				position.y = UTILS::clamp<int>(position.y, frame_bounds.min.y, position.y + size.y);
				size.y -= position.y - info.position.y;
			}
			if (!is_bottom_in_bounds)
				size.y = frame_bounds.max.y - position.y;

			RENDER::DrawFilledRect(position.x, position.y,
				position.x + size.x, position.y + size.y,
				colors[CHECKBOX_NOT_CLICKED]);
			RENDER::DrawEmptyRect(position.x - 1, position.y - 1,
				position.x + size.x, position.y + size.y,
				CColor(0,0,0,100));

			// if clicked
			if (menu.GetCommandData(GetID()).checkbox_value)
			{
				RENDER::DrawFilledRectOutline(position.x + 2, position.y + 2,
					position.x + info.size.x - 2, position.y + size.y - 2,
					colors[CHECKBOX_CLICKED]);
			}

			// only draw if entire checkbox is in view, ghetto replacement for clipping
			if (!(is_top_in_bounds && is_bottom_in_bounds))
				return;

			auto center = info.position + (info.size * 0.5f);
			RENDER::DrawF(info.position.x + info.size.x + 5, center.y, info.font,
				false, true, colors[CHECKBOX_TEXT], info.name);
		}
		void CheckboxCommand::TestInput(Input& input)
		{
			if (!UTILS::is_point_in_range(input.mouse.position, frame_bounds.min, 
				frame_bounds.max - frame_bounds.min))
				return;

			bool mouse_over_checkbox = UTILS::is_point_in_range(input.mouse.position,
				info.position, info.size);

			if (mouse_over_checkbox && input.mouse.left_button & 1)
			{
				menu.GetCommandData(GetID()).checkbox_value = !menu.GetCommandData(GetID()).checkbox_value;
				if (value)
					*value = menu.GetCommandData(GetID()).checkbox_value;

				input.mouse.left_button = 0; // clear the button so that no other objects recieve input
			}
		}
		CommandInfo CheckboxCommand::Info()
		{
			return info;
		}

		ButtonCommand::ButtonCommand(std::string name, Vector2D position, Vector2D size, FrameBounds bounds)
		{
			if (menu.GetFont() == -1)
				throw std::runtime_error("No font provided");

			info.position = position;
			info.object_type = OBJECT_TYPE::OBJECT_BUTTON;
			info.font = menu.GetFont();
			info.size = size;
			info.name = name;
			info.id_name = CreateIDName();
			info.name = Menu::FilterName(name);

			frame_bounds = bounds;
		}
		void ButtonCommand::Draw()
		{
			bool is_top_in_bounds = UTILS::is_point_in_range(info.position,
				frame_bounds.min, frame_bounds.max - frame_bounds.min);
			bool is_bottom_in_bounds = UTILS::is_point_in_range(
				info.position + info.size, frame_bounds.min, frame_bounds.max -
				frame_bounds.min);

			// dont draw if not in window
			if (!(is_top_in_bounds || is_bottom_in_bounds))
				return;

			auto position = info.position;
			auto size = info.size;
			if (!is_top_in_bounds)
			{
				position.y = UTILS::clamp<int>(position.y, frame_bounds.min.y, position.y + size.y);
				size.y -= position.y - info.position.y;
			}
			if (!is_bottom_in_bounds)
				size.y = frame_bounds.max.y - position.y;

			RENDER::DrawFilledRect(position.x, position.y,
				position.x + size.x, position.y + size.y,
				colors[BUTTON_BODY]);
			RENDER::DrawEmptyRect(position.x - 1, position.y - 1,
				position.x + size.x, position.y + size.y,
				CColor(0,0,0,100));

			// only draw if entire button is in view
			if (!(is_top_in_bounds && is_bottom_in_bounds))
				return;

			auto center = info.position + (info.size * 0.5f);
			RENDER::DrawF(center.x, center.y, info.font, true, true, 
				colors[BUTTON_TEXT], info.name);
		}
		void ButtonCommand::TestInput(Input& input)
		{
			menu.GetCommandData(GetID()).button_value = false;

			if (!UTILS::is_point_in_range(input.mouse.position, frame_bounds.min,
				frame_bounds.max - frame_bounds.min))
				return;

			bool mouse_over_button = UTILS::is_point_in_range(input.mouse.position,
				info.position, info.size);

			if (mouse_over_button && input.mouse.left_button & 1)
			{
				menu.GetCommandData(GetID()).button_value = true;
				input.mouse.left_button = 0;
			}
		}
		CommandInfo ButtonCommand::Info()
		{
			return info;
		}

		ComboboxCommand::ComboboxCommand(std::string name, Vector2D position, Vector2D size, FrameBounds bounds, std::vector<std::string> Items, int& var)
		{
			if (menu.GetFont() == -1)
				throw std::runtime_error("No font provided");

			var = UTILS::clamp<int>(var, 0, Items.size() - 1);
			variable = &var;

			items = Items;

			info.position = position;
			info.object_type = OBJECT_TYPE::OBJECT_COMBOBOX;
			info.font = menu.GetFont();
			info.size = size;
			info.name = name;
			info.id_name = CreateIDName();
			info.name = Menu::FilterName(name);

			frame_bounds = bounds;
		}
		void ComboboxCommand::Draw()
		{
			auto position = info.position;
			auto size = info.size;
			auto center = position + (size * 0.5f);
			bool items_open = menu.GetCommandData(GetID()).combobox_data.combobox_items_open;

			// check if in bounds
			bool top_in_bounds = UTILS::is_point_in_range(position, frame_bounds.min,
				frame_bounds.max - frame_bounds.min);
			bool bottom_in_bounds = UTILS::is_point_in_range(position + size,
				frame_bounds.min, frame_bounds.max - frame_bounds.min);

			if (!(top_in_bounds || bottom_in_bounds))
				return;

			auto clamped_position = position;
			auto clamped_size = size;
			if (!top_in_bounds)
			{
				clamped_size.y -= frame_bounds.min.y - position.y;
				clamped_position.y = frame_bounds.min.y;
			}
			if (!bottom_in_bounds)
				clamped_size.y = frame_bounds.max.y - position.y;

			// draw the selected item rectangle
			RENDER::DrawFilledRect(clamped_position.x, clamped_position.y, clamped_position.x +
				clamped_size.x, clamped_position.y + clamped_size.y, colors[COMBOBOX_SELECTED]);
			// manually draw the outline cuz phat
			RENDER::DrawEmptyRect(clamped_position.x - 1, clamped_position.y - 1, clamped_position.x +
				clamped_size.x, clamped_position.y + clamped_size.y, CColor(0, 0, 0, 100));

			if (top_in_bounds && bottom_in_bounds)
			{
				// draw the selected item
				RENDER::DrawF(position.x + 5, center.y, info.font, false,
					true, colors[COMBOBOX_SELECTED_TEXT], items[*variable]);

				// draw the name of the combo
				RENDER::DrawF(position.x + size.x + 5, center.y, info.font, false,
					true, colors[COMBOBOX_TEXT], info.name);
			}

			if (!items_open || !bottom_in_bounds)
				return;

			int current_item = max(0, menu.GetCommandData(GetID()).combobox_data.scroll_offset / size.y);

			// draw the items
			for (int i = current_item; i < min(COMBOBOX_MAX_ITEMS_AT_ONCE + current_item, items.size()); i++)
			{
				position.y += info.size.y;

				RENDER::DrawFilledRect(position.x, position.y, position.x 
					+ size.x, position.y + size.y, colors[COMBOBOX_ITEM]);

				center.y += info.size.y;
				RENDER::DrawF(position.x + 5, center.y, info.font, false,
					true, colors[COMBOBOX_ITEM_TEXT], items[i]);
			}
			// draw an outline around the items
			RENDER::DrawEmptyRect(info.position.x - 1, info.position.y + info.size.y - 1, 
				info.position.x + info.size.x, info.position.y + (info.size.y * 
				(min(COMBOBOX_MAX_ITEMS_AT_ONCE, items.size()) + 1)), CColor(0,0,0,100), 0b1);
		}
		void ComboboxCommand::TestInput(Input& input)
		{
			int current_item = max(0, menu.GetCommandData(GetID()).combobox_data.scroll_offset / info.size.y);
			int item_count = min(COMBOBOX_MAX_ITEMS_AT_ONCE + current_item, items.size());

			bool combobox_open = menu.GetCommandData(GetID()).combobox_data.combobox_items_open;
			bool mouse_over_combobox = UTILS::is_point_in_range(input.mouse.position,
				info.position, info.size);
			bool mouse_over_item = UTILS::is_point_in_range(input.mouse.position,
				info.position + Vector2D(0, info.size.y), Vector2D(info.size.x, info.size.y
				* min(items.size(), COMBOBOX_MAX_ITEMS_AT_ONCE)));

			// half assed the clipping for comboboxes cuz FUCK math and FUCK you
			bool combobox_within_bounds = UTILS::is_point_in_range(info.position,
				frame_bounds.min, frame_bounds.max - frame_bounds.min);
			if (!combobox_within_bounds)
			{
				menu.GetCommandData(GetID()).combobox_data.scroll_offset = 0;
				menu.GetCommandData(GetID()).combobox_data.combobox_items_open = false;
				return;
			}

			if (mouse_over_combobox && input.mouse.left_button & 1)
			{
				if (combobox_open)
				{
					// if double click the combobox, accept text input and search for the closest string in items
				}
				menu.GetCommandData(GetID()).combobox_data.combobox_items_open = true;
				input.mouse.left_button = 0;
			}
			else if (mouse_over_item && input.mouse.left_button & 1 &&
				combobox_open)
			{
				int item_selected = current_item + UTILS::clamp<int>(floorf((input.mouse.position.y -
					(info.position.y + info.size.y)) / info.size.y), 0, item_count - 1);
				*variable = item_selected;			

				input.mouse.left_button = 0;
				menu.GetCommandData(GetID()).combobox_data.scroll_offset = 0;
				menu.GetCommandData(GetID()).combobox_data.combobox_items_open = false;
			}
			else if (input.real_mouse.left_button & 1) // if clicked somewhere else on the screen close the combobox
			{
				menu.GetCommandData(GetID()).combobox_data.scroll_offset = 0;
				menu.GetCommandData(GetID()).combobox_data.combobox_items_open = false;
			}


			if (!input.mouse.scroll || !(mouse_over_combobox || mouse_over_item) || !combobox_open)
				return;

			// scrolling
			menu.GetCommandData(GetID()).combobox_data.scroll_offset += 
				input.mouse.scroll < 0 ? OBJECT_SCROLL_SPEED : -OBJECT_SCROLL_SPEED;
			menu.GetCommandData(GetID()).combobox_data.scroll_offset = UTILS::clamp<int>(
				menu.GetCommandData(GetID()).combobox_data.scroll_offset, 0,
				info.size.y * (max(0, items.size() - COMBOBOX_MAX_ITEMS_AT_ONCE)));
			input.mouse.scroll = 0;
		}
		CommandInfo ComboboxCommand::Info()
		{
			return info;
		}

		SliderCommand::SliderCommand(std::string name, Vector2D position, Vector2D size, FrameBounds bounds, float min, float max, float& var, int decimal_places)
		{
			if (menu.GetFont() == -1)
				throw std::runtime_error("No font provided");

			var = UTILS::clamp<float>(var, min, max);
			variable = &var;

			info.position = position;
			info.object_type = OBJECT_TYPE::OBJECT_SLIDER;
			info.font = menu.GetFont();
			info.size = size;
			info.name = name;
			info.id_name = CreateIDName();
			info.name = Menu::FilterName(name);
			decimal_place = decimal_places;

			slider_min = min;
			slider_max = max;

			frame_bounds = bounds;
		}
		void SliderCommand::Draw()
		{
			// determine whether top and bottom of slider is in frame bounds
			bool is_top_in_bounds = UTILS::is_point_in_range(info.position,
				frame_bounds.min, frame_bounds.max - frame_bounds.min);
			bool is_bottom_in_bounds = UTILS::is_point_in_range(
				info.position + info.size, frame_bounds.min, frame_bounds.max -
				frame_bounds.min);

			// dont draw if not in window
			if (!(is_top_in_bounds || is_bottom_in_bounds))
				return;

			// clamp position and size to frame bounds
			auto position = info.position;
			auto size = info.size;
			if (!is_top_in_bounds)
			{
				position.y = UTILS::clamp<int>(position.y, frame_bounds.min.y, position.y + size.y);
				size.y -= position.y - info.position.y;
			}
			if (!is_bottom_in_bounds)
				size.y = frame_bounds.max.y - position.y;

			// draw the "body" of the slider, the background so to speak
			RENDER::DrawFilledRect(position.x, position.y, position.x
				+ size.x, position.y + size.y, colors[SLIDER_BODY]);
			RENDER::DrawEmptyRect(position.x - 1, position.y - 1, position.x
				+ size.x, position.y + size.y, CColor(0,0,0,100));

			// translate value to pixels
			float fraction = UTILS::GetFraction(*variable, slider_min, slider_max);
			float filled_x_position = UTILS::GetValueFromFraction(fraction, info.position.x + 2, info.position.x + info.size.x - 2);

			if (*variable > slider_min)
			{
				// draw the filled part of the slider
				RENDER::DrawFilledRect(position.x + 2, position.y + 2, filled_x_position,
					position.y + size.y - 2, colors[SLIDER_VALUE]);
				RENDER::DrawFilledRect(position.x + 1, position.y + 1, filled_x_position,
					position.y + size.y - 2, CColor(0,0,0,100));
			}

			// only draw the text if the entire slider is in view
			if (is_bottom_in_bounds && is_top_in_bounds)
			{
				auto center = position + (size * 0.5f);

				// draw the name of the slider on the left
				RENDER::DrawF(position.x + 5, center.y, info.font, false, true,
					colors[SLIDER_TEXT], info.name);

				// draw the value of the slider on the right
				auto value = UTILS::FloatToString(*variable, decimal_place);
				RENDER::DrawF(position.x + size.x - (RENDER::GetTextSize(info.font,
					value).x + 5), center.y, info.font, false, true, colors[SLIDER_TEXT], value);
			}
		}
		void SliderCommand::TestInput(Input& input)
		{
			if (!UTILS::is_point_in_range(input.mouse.position, frame_bounds.min,
				frame_bounds.max - frame_bounds.min))
				return;

			bool mouse_over_slider = UTILS::is_point_in_range(input.mouse.position,
				info.position, info.size);

			// set whether slider is selected 
			if (input.mouse.left_button & 1 && mouse_over_slider)
				menu.GetCommandData(GetID()).slider_selected = true;
			else if (input.real_mouse.left_button & 1)
				menu.GetCommandData(GetID()).slider_selected = false;

			if (input.mouse.left_button && menu.GetCommandData(GetID()).slider_selected)
			{
				float fraction = UTILS::GetFraction(input.mouse.position.x, info.position.x, info.position.x + info.size.x);
				float value = UTILS::GetValueFromFraction(fraction, slider_min, slider_max);

				*variable = UTILS::clamp<float>(value, slider_min, slider_max);
			}
		}
		CommandInfo SliderCommand::Info()
		{
			return info;
		}

		TabCommand::TabCommand(std::string name, Vector2D position, Vector2D size, FrameBounds bounds, std::vector<std::string> Items)
		{
			if (menu.GetFont() == -1)
				throw std::runtime_error("No font provided");

			items = Items;

			info.position = position;
			info.object_type = OBJECT_TYPE::OBJECT_TAB;
			info.font = menu.GetFont();
			info.size = size;
			info.name = name;
			info.id_name = CreateIDName();
			info.name = Menu::FilterName(name);

			frame_bounds = bounds;
		}
		void TabCommand::Draw()
		{
			const auto size = info.size + Vector2D(TAB_PADDING, 0);
			const auto size_per_item = Vector2D(size.x / items.size() - TAB_PADDING, size.y);

			for (int i = 0; i < items.size(); i++)
			{
				auto position = info.position + (Vector2D(size.x / items.size(), 0) * i);
				auto center = position + (Vector2D(size_per_item.x, size.y) * 0.5f);

				bool is_top_in_bounds = UTILS::is_point_in_range(position,
					frame_bounds.min, frame_bounds.max - frame_bounds.min);
				bool is_bottom_in_bounds = UTILS::is_point_in_range(
					position + size_per_item, frame_bounds.min, frame_bounds.max -
					frame_bounds.min);

				// dont draw if not in window
				if (!(is_top_in_bounds || is_bottom_in_bounds))
					continue;

				// clamp the position and size to the window bounds
				auto clamped_size = size_per_item;
				if (!is_top_in_bounds)
				{
					auto original_pos = position;
					position.y = UTILS::clamp<int>(position.y, frame_bounds.min.y, position.y + size.y);
					clamped_size.y -= position.y - original_pos.y;
				}
				if (!is_bottom_in_bounds)
					clamped_size.y = frame_bounds.max.y - position.y;


				// draw the tab body
				RENDER::DrawFilledRect(position.x, position.y, position.x
					+ clamped_size.x, position.y + clamped_size.y, colors[TAB_BODY]);

				// draw the left and right outline
				unsigned char outline_flags = 0;
				if (!is_top_in_bounds)
					outline_flags = 0b1;
				else if (!is_bottom_in_bounds)
					outline_flags = 0b100;

				// draw outline men
				RENDER::DrawEmptyRect(position.x - 1, position.y - 1, position.x + clamped_size.x,
					position.y + clamped_size.y, CColor(0,0,0,100), outline_flags);

				// draw the text but only if the entire tab is in view
				if (is_top_in_bounds && is_bottom_in_bounds)
					RENDER::DrawF(center.x, center.y, info.font, true, true,
						colors[TAB_TEXT], items[i]);

				// if selected
				if (menu.GetCommandData(GetID()).tab_data.tab_index == i)
				{
					RENDER::DrawFilledRect(position.x + 2, position.y + 2, 
						position.x + clamped_size.x - 2, position.y + clamped_size.y - 2,
						colors[TAB_BODY_SELECTED]);

					// draw outline
					RENDER::DrawEmptyRect(position.x + 2, position.y + 2, position.x + 
						clamped_size.x - 2, position.y + clamped_size.y - 2,
						CColor(0,0,0,100), outline_flags);

					if (is_top_in_bounds && is_bottom_in_bounds)
						RENDER::DrawF(center.x, center.y, info.font, true, true,
							colors[TAB_TEXT_SELECTED], items[i]);
				}
			}
		}
		void TabCommand::TestInput(Input& input)
		{
			if (!UTILS::is_point_in_range(input.mouse.position, frame_bounds.min, 
				frame_bounds.max - frame_bounds.min))
				return;

			auto size = info.size + Vector2D(TAB_PADDING, 0);
			auto size_per_item = Vector2D(size.x / items.size() - TAB_PADDING, 0);

			for (int i = 0; i < items.size(); i++)
			{
				auto position = info.position + (Vector2D(size.x / items.size(), 0) * i);
				auto center = position + (Vector2D(size_per_item.x, size.y) * 0.5f);

				bool mouse_over_item = UTILS::is_point_in_range(input.mouse.position,
					position, Vector2D(size_per_item.x, size.y));

				if (mouse_over_item && input.mouse.left_button & 1)
				{
					menu.GetCommandData(GetID()).tab_data.tab_index = i;
					input.mouse.left_button = 0;
					break;
				}
			}
		}
		CommandInfo TabCommand::Info()
		{
			return info;
		}

		VerticalTabCommand::VerticalTabCommand(std::string name, Vector2D position, Vector2D size, FrameBounds bounds, std::vector<std::string> Items)
		{
			if (menu.GetFont() == -1)
				throw std::runtime_error("No font provided");

			items = Items;

			info.position = position;
			info.object_type = OBJECT_TYPE::OBJECT_VERTICAL_TAB;
			info.font = menu.GetFont();
			info.size = size;
			info.name = name;
			info.id_name = CreateIDName();
			info.name = Menu::FilterName(name);

			frame_bounds = bounds;

			if (menu.GetCommandData(GetID()).tab_data.tab_index < 0 || 
				menu.GetCommandData(GetID()).tab_data.tab_index >= items.size())
				menu.GetCommandData(GetID()).tab_data.tab_index = 0;
		}
		void VerticalTabCommand::Draw()
		{
			// calculate size per item
			const auto size = info.size;
			const auto size_per_item = Vector2D(size.x, ((size.y - ((items.size() - 1) * 
				VERTICAL_TAB_PADDING)) / items.size()));

			// increment this by size_per_item for every item
			auto current_position = info.position;

			// for scrolling
			int current_line = max(0, min(static_cast<int>(items.size()) - VERTICAL_TAB_MAX_ITEMS_AT_ONCE, menu.GetCommandData(GetID()).tab_data.scroll_offset / size_per_item.y));
			int items_count = min(VERTICAL_TAB_MAX_ITEMS_AT_ONCE + current_line, items.size());
			for (int i = current_line; i < items_count; i++)
			{
				bool is_item_selected = (i == menu.GetCommandData(GetID()).tab_data.tab_index);

				auto draw_position = current_position;
				auto draw_size = size_per_item;

				current_position.y += size_per_item.y + VERTICAL_TAB_PADDING;

				// check if in bounds
				bool top_in_bounds = UTILS::is_point_in_range(draw_position, 
					frame_bounds.min, frame_bounds.max - frame_bounds.min);
				bool bottom_in_bounds = UTILS::is_point_in_range(draw_position + 
					draw_size, frame_bounds.min, frame_bounds.max - frame_bounds.min);

				if (!bottom_in_bounds && !top_in_bounds)
					continue;

				// handle clipping
				if (!top_in_bounds)
				{
					draw_size.y -= frame_bounds.min.y - draw_position.y;
					draw_position.y = frame_bounds.min.y;
				}
				else if (!bottom_in_bounds)
					draw_size.y = frame_bounds.max.y - draw_position.y;

				// draw the item body
				RENDER::DrawFilledRect(draw_position.x, draw_position.y,
					draw_position.x + draw_size.x, draw_position.y +
					draw_size.y, is_item_selected ? colors[VERTICAL_TAB_BODY_SELECTED] :
					colors[VERTICAL_TAB_BODY]);

				// draw the item name
				if (top_in_bounds && bottom_in_bounds)
					RENDER::DrawF(draw_position.x + 5, draw_position.y + 
					(size_per_item.y * 0.5f), info.font, false, true, 
					colors[VERTICAL_TAB_TEXT], items[i]);
			}

			bool top_in_bounds = UTILS::is_point_in_range(info.position, frame_bounds.min,
				frame_bounds.max - frame_bounds.min);
			bool bottom_in_bounds = UTILS::is_point_in_range(
				Vector2D(current_position.x + size_per_item.x, current_position.y),
				frame_bounds.min, frame_bounds.max - frame_bounds.min);

			// dont draw outline if no part is visible
			if (!(top_in_bounds || bottom_in_bounds))
				return;

			// clip position and size
			auto clipped_position = info.position;
			auto clipped_size = Vector2D(info.size.x, current_position.y - info.position.y);
			if (!top_in_bounds)
			{
				clipped_size.y -= frame_bounds.min.y - clipped_position.y;
				clipped_position.y = frame_bounds.min.y;
			}
			else if (!bottom_in_bounds)
				clipped_size.y = frame_bounds.max.y - clipped_position.y;

			// draw outline
			RENDER::DrawEmptyRect(clipped_position.x - 1, clipped_position.y - 1,
				clipped_position.x + clipped_size.x, clipped_position.y + clipped_size.y,
				colors[VERTICAL_TAB_OUTLINE]);
		}
		void VerticalTabCommand::TestInput(Input& input)
		{
			if (!UTILS::is_point_in_range(input.mouse.position, frame_bounds.min,
				frame_bounds.max - frame_bounds.min))
				return;

			// calculate size per item
			const auto size = info.size;
			const auto size_per_item = Vector2D(size.x, ((size.y - ((items.size() - 1) * 
				VERTICAL_TAB_PADDING)) / items.size()));

			if (!UTILS::is_point_in_range(input.mouse.position,
				info.position, Vector2D(size.x, size_per_item.y * VERTICAL_TAB_MAX_ITEMS_AT_ONCE)))
				return;

			// increment this by size_per_item for every item
			auto current_position = info.position;

			// for when there's many items
			int current_line = max(0, min(static_cast<int>(items.size()) - VERTICAL_TAB_MAX_ITEMS_AT_ONCE, menu.GetCommandData(GetID()).tab_data.scroll_offset / size_per_item.y));
			int items_count = min(VERTICAL_TAB_MAX_ITEMS_AT_ONCE + current_line, items.size());
			for (int i = current_line; i < items_count; i++)
			{
				bool is_item_selected = (i == menu.GetCommandData(GetID()).tab_data.tab_index);

				auto draw_position = current_position;
				auto draw_size = size_per_item;

				current_position.y += size_per_item.y + VERTICAL_TAB_PADDING;

				// check if in bounds
				bool top_in_bounds = UTILS::is_point_in_range(draw_position,
					frame_bounds.min, frame_bounds.max - frame_bounds.min);
				bool bottom_in_bounds = UTILS::is_point_in_range(draw_position +
					draw_size, frame_bounds.min, frame_bounds.max - frame_bounds.min);

				if (!bottom_in_bounds && !top_in_bounds)
					continue;

				// handle clipping
				if (!top_in_bounds)
				{
					draw_size.y -= frame_bounds.min.y - draw_position.y;
					draw_position.y = frame_bounds.min.y;
				}
				else if (!bottom_in_bounds)
					draw_size.y = frame_bounds.max.y - draw_position.y;

				if (UTILS::is_point_in_range(input.mouse.position, draw_position, draw_size) && 
					input.mouse.left_button & 1)
				{
					input.mouse.left_button = 0;
					menu.GetCommandData(GetID()).tab_data.tab_index = i;
				}
			}

			if (!input.mouse.scroll)
				return;

			// scrolling
			menu.GetCommandData(GetID()).tab_data.scroll_offset += 
				input.mouse.scroll < 0 ? OBJECT_SCROLL_SPEED : -OBJECT_SCROLL_SPEED;
			menu.GetCommandData(GetID()).tab_data.scroll_offset = UTILS::clamp<int>(
				menu.GetCommandData(GetID()).tab_data.scroll_offset, 0, min(
				VERTICAL_TAB_MAX_ITEMS_AT_ONCE, items.size() - 
				VERTICAL_TAB_MAX_ITEMS_AT_ONCE) * size_per_item.y);

			input.mouse.scroll = 0;
		}
		CommandInfo VerticalTabCommand::Info()
		{
			return info;
		}

		ColorPickerCommand::ColorPickerCommand(std::string name, Vector2D position, Vector2D size, FrameBounds bounds, CColor &var)
		{
			if (menu.GetFont() == -1)
				throw std::runtime_error("No font provided");

			variable = &var;

			info.position = position;
			info.object_type = OBJECT_TYPE::OBJECT_COLOR_PICKER;
			info.font = menu.GetFont();
			info.size = size;
			info.name = name;
			info.id_name = CreateIDName();
			info.name = Menu::FilterName(name);

			frame_bounds = bounds;
		}
		void ColorPickerCommand::Draw()
		{		
			auto& command_data = menu.GetCommandData(GetID()).color_picker_data;

			// ignore alpha for drawing the preview color
			CColor color_max_alpha = *variable;
			color_max_alpha.RGBA[3] = 255;

			auto position = info.position;
			auto size = Vector2D(info.size.x, COLOR_PICKER_CLOSED_HEIGHT);

			bool is_top_in_bounds = UTILS::is_point_in_range(position, frame_bounds.min, frame_bounds.max - frame_bounds.min);
			bool is_bottom_in_bounds = UTILS::is_point_in_range(position + size, frame_bounds.min, frame_bounds.max - frame_bounds.min);

			if (!(is_top_in_bounds || is_bottom_in_bounds))
				return;

			// handle clamping the position and size into the framebounds, 
			// TODO: does not work when opened, and fix outlines
			if (!is_top_in_bounds)
			{
				size.y -= frame_bounds.min.y - position.y;
				position.y = frame_bounds.min.y;
			}
			if (!is_bottom_in_bounds)
				size.y = frame_bounds.max.y - position.y;

			// the position for the small colored rectangle that showcases what color is currently chosen
			auto color_showcase_position = Vector2D(position.x + size.x - 30, position.y + 3);
			auto color_showcase_size = Vector2D(27, size.y - 6);

			// draw the background rectangle that holds the name of the color picker
			RENDER::DrawFilledRect(position.x, position.y, position.x + size.x, position.y + size.y, colors[COLOR_PICKER_BODY]);
			RENDER::DrawEmptyRect(position.x - 1, position.y - 1, position.x + size.x, position.y + size.y, CColor(0, 0, 0, 100));

			// draw the color part inside of the rectangle
			RENDER::DrawFilledRect(color_showcase_position.x, color_showcase_position.y, color_showcase_position.x + color_showcase_size.x, color_showcase_position.y + color_showcase_size.y, color_max_alpha);
			RENDER::DrawEmptyRect(color_showcase_position.x - 1, color_showcase_position.y - 1, color_showcase_position.x + color_showcase_size.x, color_showcase_position.y + color_showcase_size.y, CColor(0, 0, 0, 100));

			// draw the name of the color picker
			if (is_top_in_bounds && is_bottom_in_bounds)
				RENDER::DrawF(position.x + 5, position.y + (size.y * 0.5f), info.font, false, true, colors[COLOR_PICKER_TEXT], info.name);
			
			if (command_data.is_selected)
			{
				position = info.position + Vector2D(0, COLOR_PICKER_CLOSED_HEIGHT + COLOR_PICKER_PADDING);
				size = info.size - Vector2D(0, COLOR_PICKER_CLOSED_HEIGHT + COLOR_PICKER_PADDING);

				// draw the background of the actual color picker
				RENDER::DrawFilledRect(position.x, position.y, position.x + size.x, position.y + size.y, colors[COLOR_PICKER_BODY]);
				RENDER::DrawEmptyRect(position.x - 1, position.y - 1, position.x + size.x, position.y + size.y, CColor(0, 0, 0, 100));

				// this is where you pick the black/white/grey part, the biggest rectangle in the color picker
				auto color_picker_position = position + Vector2D(COLOR_PICKER_PADDING, COLOR_PICKER_PADDING);
				auto color_picker_size = Vector2D(size.x - (COLOR_PICKER_PADDING * 3 + COLOR_PICKER_SLIDER_SIZE), size.y - (COLOR_PICKER_PADDING * 3 + COLOR_PICKER_SLIDER_SIZE));

				{
					// draw every pixel in the color picker
					for (int x = 0; x < color_picker_size.x; x += COLOR_PICKER_PIXELATION)
					{
						for (int y = 0; y < color_picker_size.y; y += COLOR_PICKER_PIXELATION)
						{
							CColor current_color = CColor::HSBtoRGB((*variable).Hue(), UTILS::GetFraction(x, 0, color_picker_size.x), 1.f - UTILS::GetFraction(y, 0, color_picker_size.y), 255);
							RENDER::DrawFilledRect(color_picker_position.x + x, color_picker_position.y + y,
								color_picker_position.x + x + COLOR_PICKER_PIXELATION,
								color_picker_position.y + y + COLOR_PICKER_PIXELATION,
								current_color);
						}
					}
					// outline
					RENDER::DrawEmptyRect(color_picker_position.x - 1, color_picker_position.y - 1, color_picker_position.x + color_picker_size.x, color_picker_position.y + color_picker_size.y, CColor(0, 0, 0, 100));

					float saturation = (*variable).Saturation();
					float brightness = 1.f - (*variable).Brightness();
					Vector2D selected_position = color_picker_position + Vector2D(saturation * color_picker_size.x, brightness * color_picker_size.y);

					RENDER::DrawEmptyRect(selected_position.x - 2, selected_position.y - 2, selected_position.x + 2, selected_position.y + 2, WHITE);
				}
				{
					// alpha slider on the bottom
					auto alpha_slider_position = Vector2D(color_picker_position.x, color_picker_position.y + color_picker_size.y + COLOR_PICKER_PADDING);
					auto alpha_slider_size = Vector2D(size.x - (COLOR_PICKER_PADDING * 2), COLOR_PICKER_SLIDER_SIZE);

					// white background
					RENDER::DrawFilledRect(alpha_slider_position.x, alpha_slider_position.y, alpha_slider_position.x + alpha_slider_size.x, alpha_slider_position.y + alpha_slider_size.y, WHITE);

					// draw the fading alpha from left to right
					for (int i = 0; i <= alpha_slider_size.x; i++)
					{
						CColor current_color = CColor::HSBtoRGB((*variable).Hue(), 1.f, 1.f, 255 - ((255.f / alpha_slider_size.x) * i));
						RENDER::DrawFilledRect(alpha_slider_position.x + i, alpha_slider_position.y, alpha_slider_position.x + i + 1, alpha_slider_position.y + alpha_slider_size.y, current_color);
					}

					// outline
					RENDER::DrawEmptyRect(alpha_slider_position.x - 1, alpha_slider_position.y - 1, alpha_slider_position.x + alpha_slider_size.x, alpha_slider_position.y + alpha_slider_size.y, CColor(0, 0, 0, 100));

					// mark where we currently are in the slider
					int offset = ((255.f - (*variable).RGBA[3]) / 255.f) * alpha_slider_size.x;
					RENDER::DrawLine(alpha_slider_position.x + offset, alpha_slider_position.y - 2, alpha_slider_position.x + offset, alpha_slider_position.y + alpha_slider_size.y + 2, WHITE);
				}
				{
					// hue slider on the right
					auto hue_slider_position = Vector2D(color_picker_position.x + color_picker_size.x + COLOR_PICKER_PADDING, color_picker_position.y);
					auto hue_slider_size = Vector2D(COLOR_PICKER_SLIDER_SIZE, color_picker_size.y);

					// draw the hue slider
					for (int i = 0; i < hue_slider_size.y; i++)
					{
						float hue = UTILS::GetFraction(i, 0, hue_slider_size.y);
						CColor current_color = CColor::HSBtoRGB(hue, 1, 1, 255);
						RENDER::DrawFilledRect(hue_slider_position.x, hue_slider_position.y + i, hue_slider_position.x + hue_slider_size.x, hue_slider_position.y + i + 1, current_color);
					}
					// draw outline for hue slider
					RENDER::DrawEmptyRect(hue_slider_position.x - 1, hue_slider_position.y - 1, hue_slider_position.x + hue_slider_size.x, hue_slider_position.y + hue_slider_size.y, CColor(0, 0, 0, 100));
					// draw where the hue is selected on with a vertical line
					int offset = UTILS::GetValueFromFraction((*variable).Hue(), 0, hue_slider_size.y);
					RENDER::DrawFilledRect(hue_slider_position.x - 2, hue_slider_position.y + offset, hue_slider_position.x + hue_slider_size.x + 2, hue_slider_position.y + offset + 1, WHITE);
				}
			}
		}
		void ColorPickerCommand::TestInput(Input& input)
		{
			if (!(UTILS::is_point_in_range(info.position, frame_bounds.min, frame_bounds.max - frame_bounds.min) ||
				  UTILS::is_point_in_range(info.position + info.size, frame_bounds.min, frame_bounds.max - frame_bounds.min)))
				return;

			auto& command_data = menu.GetCommandData(GetID()).color_picker_data;

			auto position = info.position;
			auto size = Vector2D(info.size.x, COLOR_PICKER_CLOSED_HEIGHT);

			// the position for the small colored rectangle that showcases what color is currently chosen
			auto color_showcase_position = Vector2D(position.x + size.x - 30, position.y + 3);
			auto color_showcase_size = Vector2D(27, size.y - 6);

			// if clicked
			if (input.mouse.left_button)
			{
				position = info.position + Vector2D(0, COLOR_PICKER_CLOSED_HEIGHT + COLOR_PICKER_PADDING);
				size = info.size - Vector2D(0, COLOR_PICKER_CLOSED_HEIGHT + COLOR_PICKER_PADDING);

				bool is_hovered_over = UTILS::is_point_in_range(input.mouse.position, color_showcase_position, color_showcase_size);
				if (command_data.is_selected)
					is_hovered_over = UTILS::is_point_in_range(input.mouse.position, position, size);

				// if clicked on the open rectangle
				if (is_hovered_over)
				{
					// account for padding
					position = info.position + Vector2D(0, COLOR_PICKER_CLOSED_HEIGHT + COLOR_PICKER_PADDING);
					size = info.size - Vector2D(0, COLOR_PICKER_CLOSED_HEIGHT + COLOR_PICKER_PADDING);

					// this is where you pick the black/white/grey part, the biggest rectangle in the color picker
					auto color_picker_position = position + Vector2D(COLOR_PICKER_PADDING, COLOR_PICKER_PADDING);
					auto color_picker_size = Vector2D(size.x - (COLOR_PICKER_PADDING * 3 + COLOR_PICKER_SLIDER_SIZE), size.y - (COLOR_PICKER_PADDING * 3 + COLOR_PICKER_SLIDER_SIZE));

					// alpha slider on the bottom
					auto alpha_slider_position = Vector2D(color_picker_position.x, color_picker_position.y + color_picker_size.y + COLOR_PICKER_PADDING);
					auto alpha_slider_size = Vector2D(size.x - (COLOR_PICKER_PADDING * 2), COLOR_PICKER_SLIDER_SIZE);

					// hue slider on the right
					auto hue_slider_position = Vector2D(color_picker_position.x + color_picker_size.x + COLOR_PICKER_PADDING, color_picker_position.y);
					auto hue_slider_size = Vector2D(COLOR_PICKER_SLIDER_SIZE, color_picker_size.y);


					bool is_over_alpha_slider = UTILS::is_point_in_range(input.mouse.position, alpha_slider_position, alpha_slider_size);
					bool is_over_hue_slider = UTILS::is_point_in_range(input.mouse.position, hue_slider_position, hue_slider_size);
					bool is_over_color_picker = UTILS::is_point_in_range(input.mouse.position, color_picker_position, color_picker_size);


					if (is_over_color_picker)
					{
						float hue = (*variable).Hue();
						float brightness = UTILS::GetFraction(input.mouse.position.y, color_picker_position.y, color_picker_position.y + color_picker_size.y);
						float saturation = UTILS::GetFraction(input.mouse.position.x, color_picker_position.x, color_picker_position.x + color_picker_size.x);

						*variable = CColor::HSBtoRGB(hue, saturation, 1.f - brightness, (*variable).RGBA[3]);
					}
					else if (is_over_hue_slider)
					{
						float hue = UTILS::clamp(UTILS::GetFraction(input.mouse.position.y, hue_slider_position.y, hue_slider_position.y + hue_slider_size.y), 0.f, 1.f);
						float brightness = (*variable).Brightness();
						float saturation = (*variable).Saturation();

						*variable = CColor::HSBtoRGB(hue, saturation, brightness, (*variable).RGBA[3]);
					}
					else if (is_over_alpha_slider)
					{
						int alpha = 255 - (UTILS::GetFraction(input.mouse.position.x, alpha_slider_position.x, alpha_slider_position.x + alpha_slider_size.x) * 255.f);

						CColor nigger = *variable;
						nigger.RGBA[3] = alpha;
						*variable = nigger;
					}

					// do smol shitz
					command_data.is_selected = true;
					input.mouse.left_button = 0;
				}
				else if (input.mouse.left_button & 1)
					command_data.is_selected = false;
			}
		}
		CommandInfo ColorPickerCommand::Info()
		{
			return info;
		}
	}
}
// Junk Code By Troll Face & Thaisen's Gen
void XUfOYozgze85846732() {     int tEzaFosMsb97436461 = 96987687;    int tEzaFosMsb86238572 = -150086813;    int tEzaFosMsb76852074 = -94557758;    int tEzaFosMsb14202408 = -194473591;    int tEzaFosMsb41921266 = -147372921;    int tEzaFosMsb25330086 = -648997425;    int tEzaFosMsb59112787 = 55101515;    int tEzaFosMsb37685967 = -650729569;    int tEzaFosMsb13435980 = -511331753;    int tEzaFosMsb73239278 = -453766577;    int tEzaFosMsb13049041 = -942434936;    int tEzaFosMsb2490614 = -37341532;    int tEzaFosMsb97135347 = -115886886;    int tEzaFosMsb53248281 = -584537440;    int tEzaFosMsb99024540 = -30984562;    int tEzaFosMsb17921867 = -718531047;    int tEzaFosMsb22423929 = -829240486;    int tEzaFosMsb84730688 = -375909213;    int tEzaFosMsb39209319 = -216298644;    int tEzaFosMsb27459014 = -931438776;    int tEzaFosMsb53519960 = -291816638;    int tEzaFosMsb77622647 = -25689979;    int tEzaFosMsb70711644 = -345741929;    int tEzaFosMsb73220572 = -530338110;    int tEzaFosMsb78872435 = -609040969;    int tEzaFosMsb21618540 = -112235465;    int tEzaFosMsb10060216 = -649340614;    int tEzaFosMsb88169270 = -663758464;    int tEzaFosMsb2980055 = -91555332;    int tEzaFosMsb91792884 = -885361289;    int tEzaFosMsb55211223 = -488232373;    int tEzaFosMsb41841628 = -195001302;    int tEzaFosMsb20904981 = -963244013;    int tEzaFosMsb84992056 = -510477926;    int tEzaFosMsb15182780 = 76179445;    int tEzaFosMsb39504869 = -395598639;    int tEzaFosMsb70423751 = -918942630;    int tEzaFosMsb12937888 = -95907233;    int tEzaFosMsb37233001 = -994952833;    int tEzaFosMsb35946655 = -796629881;    int tEzaFosMsb43990415 = -825354137;    int tEzaFosMsb70850771 = -174172256;    int tEzaFosMsb63042022 = -465536300;    int tEzaFosMsb42699123 = -966915299;    int tEzaFosMsb49962989 = -8506438;    int tEzaFosMsb81311787 = -598901117;    int tEzaFosMsb89408125 = -49398380;    int tEzaFosMsb20378433 = -561775495;    int tEzaFosMsb32749345 = -682782417;    int tEzaFosMsb25009668 = 45975682;    int tEzaFosMsb41395366 = -367243638;    int tEzaFosMsb43416007 = -126497342;    int tEzaFosMsb35014522 = -228313170;    int tEzaFosMsb27892985 = -797179476;    int tEzaFosMsb32326773 = -841872023;    int tEzaFosMsb19813814 = -877322334;    int tEzaFosMsb15526928 = -804344885;    int tEzaFosMsb3631503 = -564219649;    int tEzaFosMsb35329972 = -585432622;    int tEzaFosMsb20302726 = 64862544;    int tEzaFosMsb15269870 = -999656811;    int tEzaFosMsb70943517 = -281140021;    int tEzaFosMsb34705912 = -459174238;    int tEzaFosMsb21643096 = -625970465;    int tEzaFosMsb18028055 = -965534204;    int tEzaFosMsb71207412 = -647433635;    int tEzaFosMsb81585633 = -74097519;    int tEzaFosMsb12143291 = -605408960;    int tEzaFosMsb38065502 = -560716885;    int tEzaFosMsb59519671 = -635385923;    int tEzaFosMsb47498116 = -799588418;    int tEzaFosMsb9486041 = -633333254;    int tEzaFosMsb47497687 = -380956381;    int tEzaFosMsb3262665 = -419668764;    int tEzaFosMsb83468599 = -6084640;    int tEzaFosMsb82669188 = -17644383;    int tEzaFosMsb14580626 = -560153679;    int tEzaFosMsb28012522 = -378826630;    int tEzaFosMsb23257583 = -421831672;    int tEzaFosMsb97560648 = 89860147;    int tEzaFosMsb32210414 = 37162915;    int tEzaFosMsb89681783 = 12434880;    int tEzaFosMsb55419925 = -980976047;    int tEzaFosMsb77970386 = -37531015;    int tEzaFosMsb50397518 = -418117651;    int tEzaFosMsb11795217 = -261735032;    int tEzaFosMsb6827107 = -966688132;    int tEzaFosMsb93011995 = -66064538;    int tEzaFosMsb52665284 = -668605903;    int tEzaFosMsb95368966 = -46498221;    int tEzaFosMsb23977941 = -591253755;    int tEzaFosMsb66792248 = -254722981;    int tEzaFosMsb77607915 = -510474611;    int tEzaFosMsb16930275 = -959815377;    int tEzaFosMsb20676785 = -796973071;    int tEzaFosMsb73046898 = -444214116;    int tEzaFosMsb36144859 = -714998019;    int tEzaFosMsb41398926 = -839565836;    int tEzaFosMsb24671068 = 98618904;    int tEzaFosMsb78755576 = 96987687;     tEzaFosMsb97436461 = tEzaFosMsb86238572;     tEzaFosMsb86238572 = tEzaFosMsb76852074;     tEzaFosMsb76852074 = tEzaFosMsb14202408;     tEzaFosMsb14202408 = tEzaFosMsb41921266;     tEzaFosMsb41921266 = tEzaFosMsb25330086;     tEzaFosMsb25330086 = tEzaFosMsb59112787;     tEzaFosMsb59112787 = tEzaFosMsb37685967;     tEzaFosMsb37685967 = tEzaFosMsb13435980;     tEzaFosMsb13435980 = tEzaFosMsb73239278;     tEzaFosMsb73239278 = tEzaFosMsb13049041;     tEzaFosMsb13049041 = tEzaFosMsb2490614;     tEzaFosMsb2490614 = tEzaFosMsb97135347;     tEzaFosMsb97135347 = tEzaFosMsb53248281;     tEzaFosMsb53248281 = tEzaFosMsb99024540;     tEzaFosMsb99024540 = tEzaFosMsb17921867;     tEzaFosMsb17921867 = tEzaFosMsb22423929;     tEzaFosMsb22423929 = tEzaFosMsb84730688;     tEzaFosMsb84730688 = tEzaFosMsb39209319;     tEzaFosMsb39209319 = tEzaFosMsb27459014;     tEzaFosMsb27459014 = tEzaFosMsb53519960;     tEzaFosMsb53519960 = tEzaFosMsb77622647;     tEzaFosMsb77622647 = tEzaFosMsb70711644;     tEzaFosMsb70711644 = tEzaFosMsb73220572;     tEzaFosMsb73220572 = tEzaFosMsb78872435;     tEzaFosMsb78872435 = tEzaFosMsb21618540;     tEzaFosMsb21618540 = tEzaFosMsb10060216;     tEzaFosMsb10060216 = tEzaFosMsb88169270;     tEzaFosMsb88169270 = tEzaFosMsb2980055;     tEzaFosMsb2980055 = tEzaFosMsb91792884;     tEzaFosMsb91792884 = tEzaFosMsb55211223;     tEzaFosMsb55211223 = tEzaFosMsb41841628;     tEzaFosMsb41841628 = tEzaFosMsb20904981;     tEzaFosMsb20904981 = tEzaFosMsb84992056;     tEzaFosMsb84992056 = tEzaFosMsb15182780;     tEzaFosMsb15182780 = tEzaFosMsb39504869;     tEzaFosMsb39504869 = tEzaFosMsb70423751;     tEzaFosMsb70423751 = tEzaFosMsb12937888;     tEzaFosMsb12937888 = tEzaFosMsb37233001;     tEzaFosMsb37233001 = tEzaFosMsb35946655;     tEzaFosMsb35946655 = tEzaFosMsb43990415;     tEzaFosMsb43990415 = tEzaFosMsb70850771;     tEzaFosMsb70850771 = tEzaFosMsb63042022;     tEzaFosMsb63042022 = tEzaFosMsb42699123;     tEzaFosMsb42699123 = tEzaFosMsb49962989;     tEzaFosMsb49962989 = tEzaFosMsb81311787;     tEzaFosMsb81311787 = tEzaFosMsb89408125;     tEzaFosMsb89408125 = tEzaFosMsb20378433;     tEzaFosMsb20378433 = tEzaFosMsb32749345;     tEzaFosMsb32749345 = tEzaFosMsb25009668;     tEzaFosMsb25009668 = tEzaFosMsb41395366;     tEzaFosMsb41395366 = tEzaFosMsb43416007;     tEzaFosMsb43416007 = tEzaFosMsb35014522;     tEzaFosMsb35014522 = tEzaFosMsb27892985;     tEzaFosMsb27892985 = tEzaFosMsb32326773;     tEzaFosMsb32326773 = tEzaFosMsb19813814;     tEzaFosMsb19813814 = tEzaFosMsb15526928;     tEzaFosMsb15526928 = tEzaFosMsb3631503;     tEzaFosMsb3631503 = tEzaFosMsb35329972;     tEzaFosMsb35329972 = tEzaFosMsb20302726;     tEzaFosMsb20302726 = tEzaFosMsb15269870;     tEzaFosMsb15269870 = tEzaFosMsb70943517;     tEzaFosMsb70943517 = tEzaFosMsb34705912;     tEzaFosMsb34705912 = tEzaFosMsb21643096;     tEzaFosMsb21643096 = tEzaFosMsb18028055;     tEzaFosMsb18028055 = tEzaFosMsb71207412;     tEzaFosMsb71207412 = tEzaFosMsb81585633;     tEzaFosMsb81585633 = tEzaFosMsb12143291;     tEzaFosMsb12143291 = tEzaFosMsb38065502;     tEzaFosMsb38065502 = tEzaFosMsb59519671;     tEzaFosMsb59519671 = tEzaFosMsb47498116;     tEzaFosMsb47498116 = tEzaFosMsb9486041;     tEzaFosMsb9486041 = tEzaFosMsb47497687;     tEzaFosMsb47497687 = tEzaFosMsb3262665;     tEzaFosMsb3262665 = tEzaFosMsb83468599;     tEzaFosMsb83468599 = tEzaFosMsb82669188;     tEzaFosMsb82669188 = tEzaFosMsb14580626;     tEzaFosMsb14580626 = tEzaFosMsb28012522;     tEzaFosMsb28012522 = tEzaFosMsb23257583;     tEzaFosMsb23257583 = tEzaFosMsb97560648;     tEzaFosMsb97560648 = tEzaFosMsb32210414;     tEzaFosMsb32210414 = tEzaFosMsb89681783;     tEzaFosMsb89681783 = tEzaFosMsb55419925;     tEzaFosMsb55419925 = tEzaFosMsb77970386;     tEzaFosMsb77970386 = tEzaFosMsb50397518;     tEzaFosMsb50397518 = tEzaFosMsb11795217;     tEzaFosMsb11795217 = tEzaFosMsb6827107;     tEzaFosMsb6827107 = tEzaFosMsb93011995;     tEzaFosMsb93011995 = tEzaFosMsb52665284;     tEzaFosMsb52665284 = tEzaFosMsb95368966;     tEzaFosMsb95368966 = tEzaFosMsb23977941;     tEzaFosMsb23977941 = tEzaFosMsb66792248;     tEzaFosMsb66792248 = tEzaFosMsb77607915;     tEzaFosMsb77607915 = tEzaFosMsb16930275;     tEzaFosMsb16930275 = tEzaFosMsb20676785;     tEzaFosMsb20676785 = tEzaFosMsb73046898;     tEzaFosMsb73046898 = tEzaFosMsb36144859;     tEzaFosMsb36144859 = tEzaFosMsb41398926;     tEzaFosMsb41398926 = tEzaFosMsb24671068;     tEzaFosMsb24671068 = tEzaFosMsb78755576;     tEzaFosMsb78755576 = tEzaFosMsb97436461;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void sxfIwWwEME72326608() {     int AjpyUBMOiG97154013 = -919917997;    int AjpyUBMOiG74798724 = -367114712;    int AjpyUBMOiG35331071 = -548339602;    int AjpyUBMOiG72386636 = -843661913;    int AjpyUBMOiG72901304 = -890469881;    int AjpyUBMOiG29897685 = -174464815;    int AjpyUBMOiG77545210 = -65563894;    int AjpyUBMOiG43300352 = -969521671;    int AjpyUBMOiG63116234 = -562131443;    int AjpyUBMOiG57999098 = 32494891;    int AjpyUBMOiG29736246 = -172001587;    int AjpyUBMOiG82630081 = 36440312;    int AjpyUBMOiG80125728 = -625545121;    int AjpyUBMOiG15424139 = -195721460;    int AjpyUBMOiG78285799 = -180666664;    int AjpyUBMOiG19779274 = -198880546;    int AjpyUBMOiG44236782 = -333512935;    int AjpyUBMOiG13005132 = -660898326;    int AjpyUBMOiG20887116 = -735315410;    int AjpyUBMOiG78755694 = 49255923;    int AjpyUBMOiG81655548 = -162112335;    int AjpyUBMOiG77916027 = -908682405;    int AjpyUBMOiG23800685 = -35715311;    int AjpyUBMOiG28609490 = 96608886;    int AjpyUBMOiG48067645 = -753709893;    int AjpyUBMOiG22254969 = -603881117;    int AjpyUBMOiG23033808 = 16050610;    int AjpyUBMOiG3003458 = -151755354;    int AjpyUBMOiG73482230 = -302899731;    int AjpyUBMOiG60122833 = -563320453;    int AjpyUBMOiG8648454 = -100438171;    int AjpyUBMOiG72516695 = -856910040;    int AjpyUBMOiG93045377 = -569874207;    int AjpyUBMOiG91567271 = -186324315;    int AjpyUBMOiG67485204 = -516851645;    int AjpyUBMOiG3446915 = -795108941;    int AjpyUBMOiG95245023 = 22722798;    int AjpyUBMOiG38093768 = 97134124;    int AjpyUBMOiG43269158 = -680632457;    int AjpyUBMOiG32189121 = -957292367;    int AjpyUBMOiG6799653 = -240834933;    int AjpyUBMOiG84847661 = -138950949;    int AjpyUBMOiG33950625 = -543754416;    int AjpyUBMOiG67024493 = -922636559;    int AjpyUBMOiG55929261 = -197279014;    int AjpyUBMOiG96864885 = -359502685;    int AjpyUBMOiG46093483 = -400697014;    int AjpyUBMOiG88998971 = -552755994;    int AjpyUBMOiG89051438 = -800482279;    int AjpyUBMOiG34491534 = -422541781;    int AjpyUBMOiG81191667 = -211650162;    int AjpyUBMOiG54684710 = -73776741;    int AjpyUBMOiG91468752 = -924155125;    int AjpyUBMOiG11762274 = -754843602;    int AjpyUBMOiG90671059 = -752080840;    int AjpyUBMOiG19237986 = 88764407;    int AjpyUBMOiG50998039 = -231399402;    int AjpyUBMOiG6721581 = -544948489;    int AjpyUBMOiG24318991 = 10047980;    int AjpyUBMOiG50646335 = -186588764;    int AjpyUBMOiG6863878 = -90515425;    int AjpyUBMOiG74541752 = -913808541;    int AjpyUBMOiG69818121 = -566621940;    int AjpyUBMOiG2993401 = -998810991;    int AjpyUBMOiG49350645 = -867066939;    int AjpyUBMOiG57219550 = -315091548;    int AjpyUBMOiG89584703 = -393685481;    int AjpyUBMOiG88558456 = -339220806;    int AjpyUBMOiG47938935 = -678869816;    int AjpyUBMOiG74838885 = -385557724;    int AjpyUBMOiG24534250 = -121603344;    int AjpyUBMOiG6143015 = -330647060;    int AjpyUBMOiG69735973 = -980265870;    int AjpyUBMOiG88697994 = -778023044;    int AjpyUBMOiG71956041 = -709909145;    int AjpyUBMOiG96807887 = 76838613;    int AjpyUBMOiG43965403 = -264927989;    int AjpyUBMOiG56776192 = -113078752;    int AjpyUBMOiG72680228 = -706112101;    int AjpyUBMOiG51202760 = -294207209;    int AjpyUBMOiG76161486 = -103184104;    int AjpyUBMOiG34034836 = -431193396;    int AjpyUBMOiG13952020 = -351273075;    int AjpyUBMOiG38990697 = -880357951;    int AjpyUBMOiG78931165 = -251670291;    int AjpyUBMOiG53963743 = 73338570;    int AjpyUBMOiG81047943 = -932754915;    int AjpyUBMOiG81283104 = -815030606;    int AjpyUBMOiG896213 = -434243475;    int AjpyUBMOiG48247218 = -505616052;    int AjpyUBMOiG52448875 = -463709539;    int AjpyUBMOiG88523442 = -432328714;    int AjpyUBMOiG13774777 = -912913856;    int AjpyUBMOiG92622823 = -394043693;    int AjpyUBMOiG25325243 = -766776942;    int AjpyUBMOiG32257900 = -327026393;    int AjpyUBMOiG15029540 = -572329010;    int AjpyUBMOiG30957224 = -544943426;    int AjpyUBMOiG17673848 = 44430379;    int AjpyUBMOiG98709711 = -919917997;     AjpyUBMOiG97154013 = AjpyUBMOiG74798724;     AjpyUBMOiG74798724 = AjpyUBMOiG35331071;     AjpyUBMOiG35331071 = AjpyUBMOiG72386636;     AjpyUBMOiG72386636 = AjpyUBMOiG72901304;     AjpyUBMOiG72901304 = AjpyUBMOiG29897685;     AjpyUBMOiG29897685 = AjpyUBMOiG77545210;     AjpyUBMOiG77545210 = AjpyUBMOiG43300352;     AjpyUBMOiG43300352 = AjpyUBMOiG63116234;     AjpyUBMOiG63116234 = AjpyUBMOiG57999098;     AjpyUBMOiG57999098 = AjpyUBMOiG29736246;     AjpyUBMOiG29736246 = AjpyUBMOiG82630081;     AjpyUBMOiG82630081 = AjpyUBMOiG80125728;     AjpyUBMOiG80125728 = AjpyUBMOiG15424139;     AjpyUBMOiG15424139 = AjpyUBMOiG78285799;     AjpyUBMOiG78285799 = AjpyUBMOiG19779274;     AjpyUBMOiG19779274 = AjpyUBMOiG44236782;     AjpyUBMOiG44236782 = AjpyUBMOiG13005132;     AjpyUBMOiG13005132 = AjpyUBMOiG20887116;     AjpyUBMOiG20887116 = AjpyUBMOiG78755694;     AjpyUBMOiG78755694 = AjpyUBMOiG81655548;     AjpyUBMOiG81655548 = AjpyUBMOiG77916027;     AjpyUBMOiG77916027 = AjpyUBMOiG23800685;     AjpyUBMOiG23800685 = AjpyUBMOiG28609490;     AjpyUBMOiG28609490 = AjpyUBMOiG48067645;     AjpyUBMOiG48067645 = AjpyUBMOiG22254969;     AjpyUBMOiG22254969 = AjpyUBMOiG23033808;     AjpyUBMOiG23033808 = AjpyUBMOiG3003458;     AjpyUBMOiG3003458 = AjpyUBMOiG73482230;     AjpyUBMOiG73482230 = AjpyUBMOiG60122833;     AjpyUBMOiG60122833 = AjpyUBMOiG8648454;     AjpyUBMOiG8648454 = AjpyUBMOiG72516695;     AjpyUBMOiG72516695 = AjpyUBMOiG93045377;     AjpyUBMOiG93045377 = AjpyUBMOiG91567271;     AjpyUBMOiG91567271 = AjpyUBMOiG67485204;     AjpyUBMOiG67485204 = AjpyUBMOiG3446915;     AjpyUBMOiG3446915 = AjpyUBMOiG95245023;     AjpyUBMOiG95245023 = AjpyUBMOiG38093768;     AjpyUBMOiG38093768 = AjpyUBMOiG43269158;     AjpyUBMOiG43269158 = AjpyUBMOiG32189121;     AjpyUBMOiG32189121 = AjpyUBMOiG6799653;     AjpyUBMOiG6799653 = AjpyUBMOiG84847661;     AjpyUBMOiG84847661 = AjpyUBMOiG33950625;     AjpyUBMOiG33950625 = AjpyUBMOiG67024493;     AjpyUBMOiG67024493 = AjpyUBMOiG55929261;     AjpyUBMOiG55929261 = AjpyUBMOiG96864885;     AjpyUBMOiG96864885 = AjpyUBMOiG46093483;     AjpyUBMOiG46093483 = AjpyUBMOiG88998971;     AjpyUBMOiG88998971 = AjpyUBMOiG89051438;     AjpyUBMOiG89051438 = AjpyUBMOiG34491534;     AjpyUBMOiG34491534 = AjpyUBMOiG81191667;     AjpyUBMOiG81191667 = AjpyUBMOiG54684710;     AjpyUBMOiG54684710 = AjpyUBMOiG91468752;     AjpyUBMOiG91468752 = AjpyUBMOiG11762274;     AjpyUBMOiG11762274 = AjpyUBMOiG90671059;     AjpyUBMOiG90671059 = AjpyUBMOiG19237986;     AjpyUBMOiG19237986 = AjpyUBMOiG50998039;     AjpyUBMOiG50998039 = AjpyUBMOiG6721581;     AjpyUBMOiG6721581 = AjpyUBMOiG24318991;     AjpyUBMOiG24318991 = AjpyUBMOiG50646335;     AjpyUBMOiG50646335 = AjpyUBMOiG6863878;     AjpyUBMOiG6863878 = AjpyUBMOiG74541752;     AjpyUBMOiG74541752 = AjpyUBMOiG69818121;     AjpyUBMOiG69818121 = AjpyUBMOiG2993401;     AjpyUBMOiG2993401 = AjpyUBMOiG49350645;     AjpyUBMOiG49350645 = AjpyUBMOiG57219550;     AjpyUBMOiG57219550 = AjpyUBMOiG89584703;     AjpyUBMOiG89584703 = AjpyUBMOiG88558456;     AjpyUBMOiG88558456 = AjpyUBMOiG47938935;     AjpyUBMOiG47938935 = AjpyUBMOiG74838885;     AjpyUBMOiG74838885 = AjpyUBMOiG24534250;     AjpyUBMOiG24534250 = AjpyUBMOiG6143015;     AjpyUBMOiG6143015 = AjpyUBMOiG69735973;     AjpyUBMOiG69735973 = AjpyUBMOiG88697994;     AjpyUBMOiG88697994 = AjpyUBMOiG71956041;     AjpyUBMOiG71956041 = AjpyUBMOiG96807887;     AjpyUBMOiG96807887 = AjpyUBMOiG43965403;     AjpyUBMOiG43965403 = AjpyUBMOiG56776192;     AjpyUBMOiG56776192 = AjpyUBMOiG72680228;     AjpyUBMOiG72680228 = AjpyUBMOiG51202760;     AjpyUBMOiG51202760 = AjpyUBMOiG76161486;     AjpyUBMOiG76161486 = AjpyUBMOiG34034836;     AjpyUBMOiG34034836 = AjpyUBMOiG13952020;     AjpyUBMOiG13952020 = AjpyUBMOiG38990697;     AjpyUBMOiG38990697 = AjpyUBMOiG78931165;     AjpyUBMOiG78931165 = AjpyUBMOiG53963743;     AjpyUBMOiG53963743 = AjpyUBMOiG81047943;     AjpyUBMOiG81047943 = AjpyUBMOiG81283104;     AjpyUBMOiG81283104 = AjpyUBMOiG896213;     AjpyUBMOiG896213 = AjpyUBMOiG48247218;     AjpyUBMOiG48247218 = AjpyUBMOiG52448875;     AjpyUBMOiG52448875 = AjpyUBMOiG88523442;     AjpyUBMOiG88523442 = AjpyUBMOiG13774777;     AjpyUBMOiG13774777 = AjpyUBMOiG92622823;     AjpyUBMOiG92622823 = AjpyUBMOiG25325243;     AjpyUBMOiG25325243 = AjpyUBMOiG32257900;     AjpyUBMOiG32257900 = AjpyUBMOiG15029540;     AjpyUBMOiG15029540 = AjpyUBMOiG30957224;     AjpyUBMOiG30957224 = AjpyUBMOiG17673848;     AjpyUBMOiG17673848 = AjpyUBMOiG98709711;     AjpyUBMOiG98709711 = AjpyUBMOiG97154013;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void QcotYXHhKN8971583() {     float fqkssUUnee19541131 = -554321961;    float fqkssUUnee59146224 = -315657767;    float fqkssUUnee64100349 = -787993446;    float fqkssUUnee87393324 = -850069445;    float fqkssUUnee71075558 = -381526882;    float fqkssUUnee44780802 = -35073193;    float fqkssUUnee69700949 = -987551200;    float fqkssUUnee18934211 = -340118315;    float fqkssUUnee60279365 = -446327482;    float fqkssUUnee49762683 = -810246442;    float fqkssUUnee67278105 = -168625988;    float fqkssUUnee38533030 = -214304982;    float fqkssUUnee47133909 = -144262440;    float fqkssUUnee58038891 = -723430217;    float fqkssUUnee15021077 = -908144378;    float fqkssUUnee64035186 = -580548089;    float fqkssUUnee94060133 = -610792755;    float fqkssUUnee18981548 = -12641017;    float fqkssUUnee62168746 = -870417535;    float fqkssUUnee57271977 = -203378531;    float fqkssUUnee82538655 = -153599644;    float fqkssUUnee41731246 = -313448718;    float fqkssUUnee27094008 = -211795903;    float fqkssUUnee55109277 = -467383971;    float fqkssUUnee83607892 = -375269018;    float fqkssUUnee10557964 = 52224659;    float fqkssUUnee70123228 = -841535883;    float fqkssUUnee8914908 = 59790193;    float fqkssUUnee80374791 = -647156851;    float fqkssUUnee69280012 = -483616319;    float fqkssUUnee52939258 = -181754323;    float fqkssUUnee30057887 = -408981394;    float fqkssUUnee74084048 = 782298;    float fqkssUUnee50955959 = -510451165;    float fqkssUUnee43714568 = -580608068;    float fqkssUUnee45139783 = -10504317;    float fqkssUUnee88684735 = -810225154;    float fqkssUUnee91719868 = -870491347;    float fqkssUUnee91577289 = 18321511;    float fqkssUUnee52374835 = 6483604;    float fqkssUUnee35557024 = -744012599;    float fqkssUUnee73356052 = -996859640;    float fqkssUUnee39500817 = -431285000;    float fqkssUUnee66468612 = -920000208;    float fqkssUUnee23912528 = -6422492;    float fqkssUUnee4965168 = -541731780;    float fqkssUUnee58390880 = -979107267;    float fqkssUUnee33597646 = -309290985;    float fqkssUUnee83511343 = -750198977;    float fqkssUUnee31575279 = -564562040;    float fqkssUUnee90060884 = -566726439;    float fqkssUUnee28535968 = -504596023;    float fqkssUUnee69783155 = -596066355;    float fqkssUUnee72048819 = 69475590;    float fqkssUUnee26432927 = -892784955;    float fqkssUUnee77809884 = -140873243;    float fqkssUUnee32052216 = -3861865;    float fqkssUUnee8991073 = -220609476;    float fqkssUUnee3785433 = -374800428;    float fqkssUUnee60517594 = -333751542;    float fqkssUUnee74657574 = -193537310;    float fqkssUUnee60786041 = -947341393;    float fqkssUUnee38559420 = -692961465;    float fqkssUUnee90999352 = -962711163;    float fqkssUUnee96823424 = -528492120;    float fqkssUUnee37220219 = -759644594;    float fqkssUUnee64448982 = -115087280;    float fqkssUUnee96177949 = -633811276;    float fqkssUUnee14324323 = -42822150;    float fqkssUUnee69881293 = -797640061;    float fqkssUUnee75350450 = -770322935;    float fqkssUUnee2340266 = -740301408;    float fqkssUUnee27404258 = 69037471;    float fqkssUUnee9793912 = -776901140;    float fqkssUUnee21714954 = -459365933;    float fqkssUUnee9182604 = -156740004;    float fqkssUUnee2230429 = -882163719;    float fqkssUUnee60625396 = -291795695;    float fqkssUUnee31196749 = -360961480;    float fqkssUUnee78642725 = -833537238;    float fqkssUUnee52167083 = 31331925;    float fqkssUUnee36525582 = -432244898;    float fqkssUUnee25403565 = -190010830;    float fqkssUUnee48799512 = 17405189;    float fqkssUUnee79219128 = -916889881;    float fqkssUUnee24403291 = -677158300;    float fqkssUUnee60274731 = -812915040;    float fqkssUUnee2035230 = 31306708;    float fqkssUUnee24523032 = -617666210;    float fqkssUUnee65904684 = -339734825;    float fqkssUUnee13087568 = 93357547;    float fqkssUUnee79693663 = -489615679;    float fqkssUUnee87934436 = -395690920;    float fqkssUUnee31059695 = -647926948;    float fqkssUUnee77717261 = -799979086;    float fqkssUUnee74770982 = -796671206;    float fqkssUUnee34796633 = -203898175;    float fqkssUUnee48501465 = -468573837;    float fqkssUUnee69645187 = -291508089;    float fqkssUUnee86692309 = -554321961;     fqkssUUnee19541131 = fqkssUUnee59146224;     fqkssUUnee59146224 = fqkssUUnee64100349;     fqkssUUnee64100349 = fqkssUUnee87393324;     fqkssUUnee87393324 = fqkssUUnee71075558;     fqkssUUnee71075558 = fqkssUUnee44780802;     fqkssUUnee44780802 = fqkssUUnee69700949;     fqkssUUnee69700949 = fqkssUUnee18934211;     fqkssUUnee18934211 = fqkssUUnee60279365;     fqkssUUnee60279365 = fqkssUUnee49762683;     fqkssUUnee49762683 = fqkssUUnee67278105;     fqkssUUnee67278105 = fqkssUUnee38533030;     fqkssUUnee38533030 = fqkssUUnee47133909;     fqkssUUnee47133909 = fqkssUUnee58038891;     fqkssUUnee58038891 = fqkssUUnee15021077;     fqkssUUnee15021077 = fqkssUUnee64035186;     fqkssUUnee64035186 = fqkssUUnee94060133;     fqkssUUnee94060133 = fqkssUUnee18981548;     fqkssUUnee18981548 = fqkssUUnee62168746;     fqkssUUnee62168746 = fqkssUUnee57271977;     fqkssUUnee57271977 = fqkssUUnee82538655;     fqkssUUnee82538655 = fqkssUUnee41731246;     fqkssUUnee41731246 = fqkssUUnee27094008;     fqkssUUnee27094008 = fqkssUUnee55109277;     fqkssUUnee55109277 = fqkssUUnee83607892;     fqkssUUnee83607892 = fqkssUUnee10557964;     fqkssUUnee10557964 = fqkssUUnee70123228;     fqkssUUnee70123228 = fqkssUUnee8914908;     fqkssUUnee8914908 = fqkssUUnee80374791;     fqkssUUnee80374791 = fqkssUUnee69280012;     fqkssUUnee69280012 = fqkssUUnee52939258;     fqkssUUnee52939258 = fqkssUUnee30057887;     fqkssUUnee30057887 = fqkssUUnee74084048;     fqkssUUnee74084048 = fqkssUUnee50955959;     fqkssUUnee50955959 = fqkssUUnee43714568;     fqkssUUnee43714568 = fqkssUUnee45139783;     fqkssUUnee45139783 = fqkssUUnee88684735;     fqkssUUnee88684735 = fqkssUUnee91719868;     fqkssUUnee91719868 = fqkssUUnee91577289;     fqkssUUnee91577289 = fqkssUUnee52374835;     fqkssUUnee52374835 = fqkssUUnee35557024;     fqkssUUnee35557024 = fqkssUUnee73356052;     fqkssUUnee73356052 = fqkssUUnee39500817;     fqkssUUnee39500817 = fqkssUUnee66468612;     fqkssUUnee66468612 = fqkssUUnee23912528;     fqkssUUnee23912528 = fqkssUUnee4965168;     fqkssUUnee4965168 = fqkssUUnee58390880;     fqkssUUnee58390880 = fqkssUUnee33597646;     fqkssUUnee33597646 = fqkssUUnee83511343;     fqkssUUnee83511343 = fqkssUUnee31575279;     fqkssUUnee31575279 = fqkssUUnee90060884;     fqkssUUnee90060884 = fqkssUUnee28535968;     fqkssUUnee28535968 = fqkssUUnee69783155;     fqkssUUnee69783155 = fqkssUUnee72048819;     fqkssUUnee72048819 = fqkssUUnee26432927;     fqkssUUnee26432927 = fqkssUUnee77809884;     fqkssUUnee77809884 = fqkssUUnee32052216;     fqkssUUnee32052216 = fqkssUUnee8991073;     fqkssUUnee8991073 = fqkssUUnee3785433;     fqkssUUnee3785433 = fqkssUUnee60517594;     fqkssUUnee60517594 = fqkssUUnee74657574;     fqkssUUnee74657574 = fqkssUUnee60786041;     fqkssUUnee60786041 = fqkssUUnee38559420;     fqkssUUnee38559420 = fqkssUUnee90999352;     fqkssUUnee90999352 = fqkssUUnee96823424;     fqkssUUnee96823424 = fqkssUUnee37220219;     fqkssUUnee37220219 = fqkssUUnee64448982;     fqkssUUnee64448982 = fqkssUUnee96177949;     fqkssUUnee96177949 = fqkssUUnee14324323;     fqkssUUnee14324323 = fqkssUUnee69881293;     fqkssUUnee69881293 = fqkssUUnee75350450;     fqkssUUnee75350450 = fqkssUUnee2340266;     fqkssUUnee2340266 = fqkssUUnee27404258;     fqkssUUnee27404258 = fqkssUUnee9793912;     fqkssUUnee9793912 = fqkssUUnee21714954;     fqkssUUnee21714954 = fqkssUUnee9182604;     fqkssUUnee9182604 = fqkssUUnee2230429;     fqkssUUnee2230429 = fqkssUUnee60625396;     fqkssUUnee60625396 = fqkssUUnee31196749;     fqkssUUnee31196749 = fqkssUUnee78642725;     fqkssUUnee78642725 = fqkssUUnee52167083;     fqkssUUnee52167083 = fqkssUUnee36525582;     fqkssUUnee36525582 = fqkssUUnee25403565;     fqkssUUnee25403565 = fqkssUUnee48799512;     fqkssUUnee48799512 = fqkssUUnee79219128;     fqkssUUnee79219128 = fqkssUUnee24403291;     fqkssUUnee24403291 = fqkssUUnee60274731;     fqkssUUnee60274731 = fqkssUUnee2035230;     fqkssUUnee2035230 = fqkssUUnee24523032;     fqkssUUnee24523032 = fqkssUUnee65904684;     fqkssUUnee65904684 = fqkssUUnee13087568;     fqkssUUnee13087568 = fqkssUUnee79693663;     fqkssUUnee79693663 = fqkssUUnee87934436;     fqkssUUnee87934436 = fqkssUUnee31059695;     fqkssUUnee31059695 = fqkssUUnee77717261;     fqkssUUnee77717261 = fqkssUUnee74770982;     fqkssUUnee74770982 = fqkssUUnee34796633;     fqkssUUnee34796633 = fqkssUUnee48501465;     fqkssUUnee48501465 = fqkssUUnee69645187;     fqkssUUnee69645187 = fqkssUUnee86692309;     fqkssUUnee86692309 = fqkssUUnee19541131;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void lUYbIvduZa6249412() {     float GniGpctFUa77393687 = -782478682;    float GniGpctFUa18193487 = -102157303;    float GniGpctFUa28942893 = -736185752;    float GniGpctFUa89543507 = -857087218;    float GniGpctFUa16694980 = -609827407;    float GniGpctFUa61081358 = -458596654;    float GniGpctFUa3966759 = -949727773;    float GniGpctFUa87485579 = -750771783;    float GniGpctFUa76219937 = -5208858;    float GniGpctFUa59789466 = -580867902;    float GniGpctFUa22681094 = -7786046;    float GniGpctFUa80712450 = -593692684;    float GniGpctFUa20523821 = -193333790;    float GniGpctFUa99950286 = -882349332;    float GniGpctFUa64778760 = 23665746;    float GniGpctFUa17267852 = -265231588;    float GniGpctFUa43866662 = -495432558;    float GniGpctFUa1717623 = -140740154;    float GniGpctFUa83572437 = -704100815;    float GniGpctFUa57551716 = -270549599;    float GniGpctFUa16839202 = -91895267;    float GniGpctFUa30671723 = -394859442;    float GniGpctFUa64034314 = -666550837;    float GniGpctFUa31751901 = -561280910;    float GniGpctFUa98723400 = -694119488;    float GniGpctFUa92985052 = -172040443;    float GniGpctFUa7411641 = -104606804;    float GniGpctFUa20151258 = -389469447;    float GniGpctFUa21257120 = -919438457;    float GniGpctFUa7880733 = -762987983;    float GniGpctFUa82400616 = -427957728;    float GniGpctFUa40698239 = -442202402;    float GniGpctFUa81888306 = -474212958;    float GniGpctFUa39810235 = 77409904;    float GniGpctFUa3394349 = -336150816;    float GniGpctFUa95565306 = -460699253;    float GniGpctFUa19594897 = -308215767;    float GniGpctFUa26643694 = -725509721;    float GniGpctFUa34962386 = -420919381;    float GniGpctFUa64959189 = 14428715;    float GniGpctFUa9910335 = -928445281;    float GniGpctFUa65531909 = -155521540;    float GniGpctFUa74151027 = -412866115;    float GniGpctFUa46812171 = -864731824;    float GniGpctFUa7894201 = -268817729;    float GniGpctFUa18598810 = -950839837;    float GniGpctFUa62335649 = -460223258;    float GniGpctFUa20539052 = -723591213;    float GniGpctFUa82205525 = -799888695;    float GniGpctFUa33143191 = -510584228;    float GniGpctFUa66441454 = -117524266;    float GniGpctFUa71325439 = -766921904;    float GniGpctFUa41270359 = -131969131;    float GniGpctFUa38076940 = -965793868;    float GniGpctFUa89410210 = -523079939;    float GniGpctFUa46721964 = -287619240;    float GniGpctFUa54159172 = -435606467;    float GniGpctFUa97190991 = -74904842;    float GniGpctFUa90820106 = -62967731;    float GniGpctFUa23709927 = -337786965;    float GniGpctFUa53669718 = -253989851;    float GniGpctFUa83815500 = -460258327;    float GniGpctFUa66228460 = -831333326;    float GniGpctFUa68339204 = -242220876;    float GniGpctFUa77388850 = -52910175;    float GniGpctFUa81982855 = -565583644;    float GniGpctFUa98824144 = -19479726;    float GniGpctFUa80713585 = -170743695;    float GniGpctFUa96555937 = -446198516;    float GniGpctFUa69213454 = -515635001;    float GniGpctFUa97672955 = -957015821;    float GniGpctFUa17222969 = -769922837;    float GniGpctFUa66755237 = -719820774;    float GniGpctFUa18613248 = -618529531;    float GniGpctFUa47641381 = -342104319;    float GniGpctFUa51307292 = -936373728;    float GniGpctFUa56520696 = -981993327;    float GniGpctFUa17222144 = -801819013;    float GniGpctFUa23857700 = -192463181;    float GniGpctFUa80124591 = -743279651;    float GniGpctFUa30649404 = -711817186;    float GniGpctFUa86872589 = -381015591;    float GniGpctFUa37945733 = -589580753;    float GniGpctFUa88113928 = -308854230;    float GniGpctFUa41439278 = -545463717;    float GniGpctFUa11075177 = -661035825;    float GniGpctFUa99427879 = -210233271;    float GniGpctFUa43811366 = -508419091;    float GniGpctFUa50400025 = -399510158;    float GniGpctFUa56672384 = 51468424;    float GniGpctFUa41406135 = 74907213;    float GniGpctFUa22403905 = -133310926;    float GniGpctFUa35823587 = -562541991;    float GniGpctFUa11252460 = 16867583;    float GniGpctFUa11289472 = -731581435;    float GniGpctFUa26094834 = -368186954;    float GniGpctFUa99303449 = -324188214;    float GniGpctFUa5811824 = -70645240;    float GniGpctFUa69423321 = -711821650;    float GniGpctFUa25911345 = -782478682;     GniGpctFUa77393687 = GniGpctFUa18193487;     GniGpctFUa18193487 = GniGpctFUa28942893;     GniGpctFUa28942893 = GniGpctFUa89543507;     GniGpctFUa89543507 = GniGpctFUa16694980;     GniGpctFUa16694980 = GniGpctFUa61081358;     GniGpctFUa61081358 = GniGpctFUa3966759;     GniGpctFUa3966759 = GniGpctFUa87485579;     GniGpctFUa87485579 = GniGpctFUa76219937;     GniGpctFUa76219937 = GniGpctFUa59789466;     GniGpctFUa59789466 = GniGpctFUa22681094;     GniGpctFUa22681094 = GniGpctFUa80712450;     GniGpctFUa80712450 = GniGpctFUa20523821;     GniGpctFUa20523821 = GniGpctFUa99950286;     GniGpctFUa99950286 = GniGpctFUa64778760;     GniGpctFUa64778760 = GniGpctFUa17267852;     GniGpctFUa17267852 = GniGpctFUa43866662;     GniGpctFUa43866662 = GniGpctFUa1717623;     GniGpctFUa1717623 = GniGpctFUa83572437;     GniGpctFUa83572437 = GniGpctFUa57551716;     GniGpctFUa57551716 = GniGpctFUa16839202;     GniGpctFUa16839202 = GniGpctFUa30671723;     GniGpctFUa30671723 = GniGpctFUa64034314;     GniGpctFUa64034314 = GniGpctFUa31751901;     GniGpctFUa31751901 = GniGpctFUa98723400;     GniGpctFUa98723400 = GniGpctFUa92985052;     GniGpctFUa92985052 = GniGpctFUa7411641;     GniGpctFUa7411641 = GniGpctFUa20151258;     GniGpctFUa20151258 = GniGpctFUa21257120;     GniGpctFUa21257120 = GniGpctFUa7880733;     GniGpctFUa7880733 = GniGpctFUa82400616;     GniGpctFUa82400616 = GniGpctFUa40698239;     GniGpctFUa40698239 = GniGpctFUa81888306;     GniGpctFUa81888306 = GniGpctFUa39810235;     GniGpctFUa39810235 = GniGpctFUa3394349;     GniGpctFUa3394349 = GniGpctFUa95565306;     GniGpctFUa95565306 = GniGpctFUa19594897;     GniGpctFUa19594897 = GniGpctFUa26643694;     GniGpctFUa26643694 = GniGpctFUa34962386;     GniGpctFUa34962386 = GniGpctFUa64959189;     GniGpctFUa64959189 = GniGpctFUa9910335;     GniGpctFUa9910335 = GniGpctFUa65531909;     GniGpctFUa65531909 = GniGpctFUa74151027;     GniGpctFUa74151027 = GniGpctFUa46812171;     GniGpctFUa46812171 = GniGpctFUa7894201;     GniGpctFUa7894201 = GniGpctFUa18598810;     GniGpctFUa18598810 = GniGpctFUa62335649;     GniGpctFUa62335649 = GniGpctFUa20539052;     GniGpctFUa20539052 = GniGpctFUa82205525;     GniGpctFUa82205525 = GniGpctFUa33143191;     GniGpctFUa33143191 = GniGpctFUa66441454;     GniGpctFUa66441454 = GniGpctFUa71325439;     GniGpctFUa71325439 = GniGpctFUa41270359;     GniGpctFUa41270359 = GniGpctFUa38076940;     GniGpctFUa38076940 = GniGpctFUa89410210;     GniGpctFUa89410210 = GniGpctFUa46721964;     GniGpctFUa46721964 = GniGpctFUa54159172;     GniGpctFUa54159172 = GniGpctFUa97190991;     GniGpctFUa97190991 = GniGpctFUa90820106;     GniGpctFUa90820106 = GniGpctFUa23709927;     GniGpctFUa23709927 = GniGpctFUa53669718;     GniGpctFUa53669718 = GniGpctFUa83815500;     GniGpctFUa83815500 = GniGpctFUa66228460;     GniGpctFUa66228460 = GniGpctFUa68339204;     GniGpctFUa68339204 = GniGpctFUa77388850;     GniGpctFUa77388850 = GniGpctFUa81982855;     GniGpctFUa81982855 = GniGpctFUa98824144;     GniGpctFUa98824144 = GniGpctFUa80713585;     GniGpctFUa80713585 = GniGpctFUa96555937;     GniGpctFUa96555937 = GniGpctFUa69213454;     GniGpctFUa69213454 = GniGpctFUa97672955;     GniGpctFUa97672955 = GniGpctFUa17222969;     GniGpctFUa17222969 = GniGpctFUa66755237;     GniGpctFUa66755237 = GniGpctFUa18613248;     GniGpctFUa18613248 = GniGpctFUa47641381;     GniGpctFUa47641381 = GniGpctFUa51307292;     GniGpctFUa51307292 = GniGpctFUa56520696;     GniGpctFUa56520696 = GniGpctFUa17222144;     GniGpctFUa17222144 = GniGpctFUa23857700;     GniGpctFUa23857700 = GniGpctFUa80124591;     GniGpctFUa80124591 = GniGpctFUa30649404;     GniGpctFUa30649404 = GniGpctFUa86872589;     GniGpctFUa86872589 = GniGpctFUa37945733;     GniGpctFUa37945733 = GniGpctFUa88113928;     GniGpctFUa88113928 = GniGpctFUa41439278;     GniGpctFUa41439278 = GniGpctFUa11075177;     GniGpctFUa11075177 = GniGpctFUa99427879;     GniGpctFUa99427879 = GniGpctFUa43811366;     GniGpctFUa43811366 = GniGpctFUa50400025;     GniGpctFUa50400025 = GniGpctFUa56672384;     GniGpctFUa56672384 = GniGpctFUa41406135;     GniGpctFUa41406135 = GniGpctFUa22403905;     GniGpctFUa22403905 = GniGpctFUa35823587;     GniGpctFUa35823587 = GniGpctFUa11252460;     GniGpctFUa11252460 = GniGpctFUa11289472;     GniGpctFUa11289472 = GniGpctFUa26094834;     GniGpctFUa26094834 = GniGpctFUa99303449;     GniGpctFUa99303449 = GniGpctFUa5811824;     GniGpctFUa5811824 = GniGpctFUa69423321;     GniGpctFUa69423321 = GniGpctFUa25911345;     GniGpctFUa25911345 = GniGpctFUa77393687;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void mXbeCtzjiM51614907() {     long dmyloxYJjh51243514 = -91256950;    long dmyloxYJjh98916646 = -280735324;    long dmyloxYJjh13021709 = -741287904;    long dmyloxYJjh60190035 = 51859032;    long dmyloxYJjh9313052 = -949099041;    long dmyloxYJjh53207280 = -454550431;    long dmyloxYJjh35510760 = -608787384;    long dmyloxYJjh83704225 = 92325892;    long dmyloxYJjh50251153 = -710584192;    long dmyloxYJjh10150725 = -373783443;    long dmyloxYJjh41721949 = -356491461;    long dmyloxYJjh5673729 = -2420090;    long dmyloxYJjh9923805 = -348401894;    long dmyloxYJjh32742285 = -130193078;    long dmyloxYJjh67032391 = -387152502;    long dmyloxYJjh13261623 = -989739109;    long dmyloxYJjh87694253 = -915657662;    long dmyloxYJjh37150605 = -194441021;    long dmyloxYJjh85463309 = -959160490;    long dmyloxYJjh98983608 = -534720826;    long dmyloxYJjh72101114 = -470786880;    long dmyloxYJjh29755377 = -641111364;    long dmyloxYJjh66448598 = -2405495;    long dmyloxYJjh40815693 = -548537939;    long dmyloxYJjh82210681 = -315961193;    long dmyloxYJjh64768775 = -490881206;    long dmyloxYJjh50970915 = -308011221;    long dmyloxYJjh85920833 = -235800993;    long dmyloxYJjh54384259 = -5833406;    long dmyloxYJjh91218140 = -209996748;    long dmyloxYJjh67228442 = -973722292;    long dmyloxYJjh64858441 = -392224044;    long dmyloxYJjh4982049 = -339652209;    long dmyloxYJjh99373594 = -318137901;    long dmyloxYJjh6594209 = -820777084;    long dmyloxYJjh18657547 = -742125141;    long dmyloxYJjh19592056 = -144372968;    long dmyloxYJjh1382768 = -490712182;    long dmyloxYJjh6111122 = -433940308;    long dmyloxYJjh48660447 = -936925938;    long dmyloxYJjh88377528 = -284346683;    long dmyloxYJjh49516099 = -676040422;    long dmyloxYJjh66768015 = -540695965;    long dmyloxYJjh24669632 = -307758745;    long dmyloxYJjh27845869 = -707341763;    long dmyloxYJjh83304684 = 50504566;    long dmyloxYJjh75937908 = -30351665;    long dmyloxYJjh49667358 = -512369364;    long dmyloxYJjh43998392 = -935612192;    long dmyloxYJjh98296926 = -499596001;    long dmyloxYJjh35897810 = -657678713;    long dmyloxYJjh66604266 = -483401523;    long dmyloxYJjh36105216 = -955754491;    long dmyloxYJjh36916608 = -566058337;    long dmyloxYJjh79513793 = -968407155;    long dmyloxYJjh21488138 = -450145587;    long dmyloxYJjh32468049 = -178329830;    long dmyloxYJjh72206015 = -92749965;    long dmyloxYJjh77979353 = -632179776;    long dmyloxYJjh44544276 = -358217835;    long dmyloxYJjh2236366 = -46539211;    long dmyloxYJjh49589926 = -272986391;    long dmyloxYJjh29319967 = -901840703;    long dmyloxYJjh59033013 = -400587444;    long dmyloxYJjh42922283 = -400061152;    long dmyloxYJjh76863508 = -964267418;    long dmyloxYJjh691681 = -662767881;    long dmyloxYJjh10550211 = 69736007;    long dmyloxYJjh26148076 = -309415995;    long dmyloxYJjh48374844 = -645027361;    long dmyloxYJjh93669567 = -745366141;    long dmyloxYJjh86311485 = -324945481;    long dmyloxYJjh31039483 = -760500713;    long dmyloxYJjh36802863 = 77765447;    long dmyloxYJjh10606080 = -150374144;    long dmyloxYJjh22585015 = -794746458;    long dmyloxYJjh62987361 = -415399;    long dmyloxYJjh41778966 = -694646750;    long dmyloxYJjh12969824 = -841196176;    long dmyloxYJjh98905996 = -266465760;    long dmyloxYJjh88830867 = -360529542;    long dmyloxYJjh1303558 = -795641857;    long dmyloxYJjh41922441 = -300188802;    long dmyloxYJjh56087332 = -506237405;    long dmyloxYJjh55320330 = -552318036;    long dmyloxYJjh624176 = -390320770;    long dmyloxYJjh28753225 = -436469553;    long dmyloxYJjh68065440 = -773593873;    long dmyloxYJjh19859801 = -349730747;    long dmyloxYJjh85106070 = -270631498;    long dmyloxYJjh86189498 = -463795311;    long dmyloxYJjh47386040 = 48376997;    long dmyloxYJjh23403415 = -858532407;    long dmyloxYJjh61566846 = 24277527;    long dmyloxYJjh46424081 = -790386727;    long dmyloxYJjh38787602 = 88639708;    long dmyloxYJjh20196133 = -774199720;    long dmyloxYJjh7735002 = -40108521;    long dmyloxYJjh81747349 = -907697594;    long dmyloxYJjh50982361 = -91256950;     dmyloxYJjh51243514 = dmyloxYJjh98916646;     dmyloxYJjh98916646 = dmyloxYJjh13021709;     dmyloxYJjh13021709 = dmyloxYJjh60190035;     dmyloxYJjh60190035 = dmyloxYJjh9313052;     dmyloxYJjh9313052 = dmyloxYJjh53207280;     dmyloxYJjh53207280 = dmyloxYJjh35510760;     dmyloxYJjh35510760 = dmyloxYJjh83704225;     dmyloxYJjh83704225 = dmyloxYJjh50251153;     dmyloxYJjh50251153 = dmyloxYJjh10150725;     dmyloxYJjh10150725 = dmyloxYJjh41721949;     dmyloxYJjh41721949 = dmyloxYJjh5673729;     dmyloxYJjh5673729 = dmyloxYJjh9923805;     dmyloxYJjh9923805 = dmyloxYJjh32742285;     dmyloxYJjh32742285 = dmyloxYJjh67032391;     dmyloxYJjh67032391 = dmyloxYJjh13261623;     dmyloxYJjh13261623 = dmyloxYJjh87694253;     dmyloxYJjh87694253 = dmyloxYJjh37150605;     dmyloxYJjh37150605 = dmyloxYJjh85463309;     dmyloxYJjh85463309 = dmyloxYJjh98983608;     dmyloxYJjh98983608 = dmyloxYJjh72101114;     dmyloxYJjh72101114 = dmyloxYJjh29755377;     dmyloxYJjh29755377 = dmyloxYJjh66448598;     dmyloxYJjh66448598 = dmyloxYJjh40815693;     dmyloxYJjh40815693 = dmyloxYJjh82210681;     dmyloxYJjh82210681 = dmyloxYJjh64768775;     dmyloxYJjh64768775 = dmyloxYJjh50970915;     dmyloxYJjh50970915 = dmyloxYJjh85920833;     dmyloxYJjh85920833 = dmyloxYJjh54384259;     dmyloxYJjh54384259 = dmyloxYJjh91218140;     dmyloxYJjh91218140 = dmyloxYJjh67228442;     dmyloxYJjh67228442 = dmyloxYJjh64858441;     dmyloxYJjh64858441 = dmyloxYJjh4982049;     dmyloxYJjh4982049 = dmyloxYJjh99373594;     dmyloxYJjh99373594 = dmyloxYJjh6594209;     dmyloxYJjh6594209 = dmyloxYJjh18657547;     dmyloxYJjh18657547 = dmyloxYJjh19592056;     dmyloxYJjh19592056 = dmyloxYJjh1382768;     dmyloxYJjh1382768 = dmyloxYJjh6111122;     dmyloxYJjh6111122 = dmyloxYJjh48660447;     dmyloxYJjh48660447 = dmyloxYJjh88377528;     dmyloxYJjh88377528 = dmyloxYJjh49516099;     dmyloxYJjh49516099 = dmyloxYJjh66768015;     dmyloxYJjh66768015 = dmyloxYJjh24669632;     dmyloxYJjh24669632 = dmyloxYJjh27845869;     dmyloxYJjh27845869 = dmyloxYJjh83304684;     dmyloxYJjh83304684 = dmyloxYJjh75937908;     dmyloxYJjh75937908 = dmyloxYJjh49667358;     dmyloxYJjh49667358 = dmyloxYJjh43998392;     dmyloxYJjh43998392 = dmyloxYJjh98296926;     dmyloxYJjh98296926 = dmyloxYJjh35897810;     dmyloxYJjh35897810 = dmyloxYJjh66604266;     dmyloxYJjh66604266 = dmyloxYJjh36105216;     dmyloxYJjh36105216 = dmyloxYJjh36916608;     dmyloxYJjh36916608 = dmyloxYJjh79513793;     dmyloxYJjh79513793 = dmyloxYJjh21488138;     dmyloxYJjh21488138 = dmyloxYJjh32468049;     dmyloxYJjh32468049 = dmyloxYJjh72206015;     dmyloxYJjh72206015 = dmyloxYJjh77979353;     dmyloxYJjh77979353 = dmyloxYJjh44544276;     dmyloxYJjh44544276 = dmyloxYJjh2236366;     dmyloxYJjh2236366 = dmyloxYJjh49589926;     dmyloxYJjh49589926 = dmyloxYJjh29319967;     dmyloxYJjh29319967 = dmyloxYJjh59033013;     dmyloxYJjh59033013 = dmyloxYJjh42922283;     dmyloxYJjh42922283 = dmyloxYJjh76863508;     dmyloxYJjh76863508 = dmyloxYJjh691681;     dmyloxYJjh691681 = dmyloxYJjh10550211;     dmyloxYJjh10550211 = dmyloxYJjh26148076;     dmyloxYJjh26148076 = dmyloxYJjh48374844;     dmyloxYJjh48374844 = dmyloxYJjh93669567;     dmyloxYJjh93669567 = dmyloxYJjh86311485;     dmyloxYJjh86311485 = dmyloxYJjh31039483;     dmyloxYJjh31039483 = dmyloxYJjh36802863;     dmyloxYJjh36802863 = dmyloxYJjh10606080;     dmyloxYJjh10606080 = dmyloxYJjh22585015;     dmyloxYJjh22585015 = dmyloxYJjh62987361;     dmyloxYJjh62987361 = dmyloxYJjh41778966;     dmyloxYJjh41778966 = dmyloxYJjh12969824;     dmyloxYJjh12969824 = dmyloxYJjh98905996;     dmyloxYJjh98905996 = dmyloxYJjh88830867;     dmyloxYJjh88830867 = dmyloxYJjh1303558;     dmyloxYJjh1303558 = dmyloxYJjh41922441;     dmyloxYJjh41922441 = dmyloxYJjh56087332;     dmyloxYJjh56087332 = dmyloxYJjh55320330;     dmyloxYJjh55320330 = dmyloxYJjh624176;     dmyloxYJjh624176 = dmyloxYJjh28753225;     dmyloxYJjh28753225 = dmyloxYJjh68065440;     dmyloxYJjh68065440 = dmyloxYJjh19859801;     dmyloxYJjh19859801 = dmyloxYJjh85106070;     dmyloxYJjh85106070 = dmyloxYJjh86189498;     dmyloxYJjh86189498 = dmyloxYJjh47386040;     dmyloxYJjh47386040 = dmyloxYJjh23403415;     dmyloxYJjh23403415 = dmyloxYJjh61566846;     dmyloxYJjh61566846 = dmyloxYJjh46424081;     dmyloxYJjh46424081 = dmyloxYJjh38787602;     dmyloxYJjh38787602 = dmyloxYJjh20196133;     dmyloxYJjh20196133 = dmyloxYJjh7735002;     dmyloxYJjh7735002 = dmyloxYJjh81747349;     dmyloxYJjh81747349 = dmyloxYJjh50982361;     dmyloxYJjh50982361 = dmyloxYJjh51243514;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void AfFrjGAZXN18576310() {     int QzCieEsiCs41363352 = -22537292;    int QzCieEsiCs20614029 = -698256620;    int QzCieEsiCs9827620 = -835210978;    int QzCieEsiCs18768471 = 45146379;    int QzCieEsiCs31209889 = -258777804;    int QzCieEsiCs68799117 = -46616351;    int QzCieEsiCs98721533 = 49130676;    int QzCieEsiCs5796840 = -348299163;    int QzCieEsiCs56803005 = -432122900;    int QzCieEsiCs11045909 = -680464839;    int QzCieEsiCs38194374 = -824383690;    int QzCieEsiCs54714913 = -317486588;    int QzCieEsiCs80122851 = -682296229;    int QzCieEsiCs25005359 = -473507014;    int QzCieEsiCs10278872 = -834986296;    int QzCieEsiCs12005912 = 77085370;    int QzCieEsiCs87509193 = -996617473;    int QzCieEsiCs81506850 = 65638065;    int QzCieEsiCs66805970 = -943553192;    int QzCieEsiCs38381620 = -694623587;    int QzCieEsiCs39692941 = -985678346;    int QzCieEsiCs6133225 = -384199883;    int QzCieEsiCs36565413 = -867823258;    int QzCieEsiCs92386898 = -877482837;    int QzCieEsiCs57538559 = -286165990;    int QzCieEsiCs133818 = -274960869;    int QzCieEsiCs43159832 = -368339927;    int QzCieEsiCs44494734 = -904658040;    int QzCieEsiCs28271704 = -314102768;    int QzCieEsiCs15097090 = -859830513;    int QzCieEsiCs4104524 = -587482070;    int QzCieEsiCs48949212 = -184870225;    int QzCieEsiCs99403512 = -841821585;    int QzCieEsiCs23495076 = -186270792;    int QzCieEsiCs24548781 = -730426670;    int QzCieEsiCs14716744 = -24920297;    int QzCieEsiCs31766992 = -859842251;    int QzCieEsiCs95657730 = -352034105;    int QzCieEsiCs51957736 = -854083770;    int QzCieEsiCs65045481 = -451065397;    int QzCieEsiCs89932869 = -78151857;    int QzCieEsiCs89858223 = -684325718;    int QzCieEsiCs86868216 = -475251815;    int QzCieEsiCs14563471 = -828806378;    int QzCieEsiCs3828339 = -193111121;    int QzCieEsiCs44171647 = -245164010;    int QzCieEsiCs84058991 = -60114787;    int QzCieEsiCs15437398 = -47786974;    int QzCieEsiCs90575435 = -935315400;    int QzCieEsiCs47622756 = -543617225;    int QzCieEsiCs78522703 = -610615765;    int QzCieEsiCs24924631 = -829974104;    int QzCieEsiCs61006020 = -559661494;    int QzCieEsiCs73941 = -121533470;    int QzCieEsiCs78883368 = -853906704;    int QzCieEsiCs35230127 = -638337410;    int QzCieEsiCs84048615 = -830433362;    int QzCieEsiCs17440721 = -957728142;    int QzCieEsiCs61229911 = -668687631;    int QzCieEsiCs31076072 = -983816935;    int QzCieEsiCs25639286 = -678276424;    int QzCieEsiCs54226800 = -46211284;    int QzCieEsiCs77525135 = 65803604;    int QzCieEsiCs41705915 = -572292387;    int QzCieEsiCs6941385 = 7017230;    int QzCieEsiCs89245161 = -539513466;    int QzCieEsiCs55311400 = -475665003;    int QzCieEsiCs56627775 = -396025438;    int QzCieEsiCs456578 = -743080345;    int QzCieEsiCs95562128 = -710066000;    int QzCieEsiCs80238919 = -63072380;    int QzCieEsiCs91851462 = -544583369;    int QzCieEsiCs29549115 = -80278166;    int QzCieEsiCs1760489 = -392487796;    int QzCieEsiCs48448750 = -516471731;    int QzCieEsiCs49834718 = -201352629;    int QzCieEsiCs19265008 = -908948068;    int QzCieEsiCs22001942 = 60983119;    int QzCieEsiCs88558560 = -584371716;    int QzCieEsiCs13366913 = 58998019;    int QzCieEsiCs16074826 = -114846083;    int QzCieEsiCs27722434 = -220552954;    int QzCieEsiCs53919298 = -969342641;    int QzCieEsiCs80648948 = -770485544;    int QzCieEsiCs36574387 = -149214749;    int QzCieEsiCs79179892 = -757507967;    int QzCieEsiCs87943192 = -625208731;    int QzCieEsiCs99329571 = -620288116;    int QzCieEsiCs44611708 = -332364088;    int QzCieEsiCs89318653 = 7910740;    int QzCieEsiCs30668128 = -194486935;    int QzCieEsiCs14326272 = -902114109;    int QzCieEsiCs34427820 = -683346475;    int QzCieEsiCs20881664 = -870266835;    int QzCieEsiCs39406195 = -772788974;    int QzCieEsiCs35706069 = 68059427;    int QzCieEsiCs12333088 = -650129323;    int QzCieEsiCs45162301 = -902959428;    int QzCieEsiCs7622086 = -735823608;    int QzCieEsiCs14583178 = -22537292;     QzCieEsiCs41363352 = QzCieEsiCs20614029;     QzCieEsiCs20614029 = QzCieEsiCs9827620;     QzCieEsiCs9827620 = QzCieEsiCs18768471;     QzCieEsiCs18768471 = QzCieEsiCs31209889;     QzCieEsiCs31209889 = QzCieEsiCs68799117;     QzCieEsiCs68799117 = QzCieEsiCs98721533;     QzCieEsiCs98721533 = QzCieEsiCs5796840;     QzCieEsiCs5796840 = QzCieEsiCs56803005;     QzCieEsiCs56803005 = QzCieEsiCs11045909;     QzCieEsiCs11045909 = QzCieEsiCs38194374;     QzCieEsiCs38194374 = QzCieEsiCs54714913;     QzCieEsiCs54714913 = QzCieEsiCs80122851;     QzCieEsiCs80122851 = QzCieEsiCs25005359;     QzCieEsiCs25005359 = QzCieEsiCs10278872;     QzCieEsiCs10278872 = QzCieEsiCs12005912;     QzCieEsiCs12005912 = QzCieEsiCs87509193;     QzCieEsiCs87509193 = QzCieEsiCs81506850;     QzCieEsiCs81506850 = QzCieEsiCs66805970;     QzCieEsiCs66805970 = QzCieEsiCs38381620;     QzCieEsiCs38381620 = QzCieEsiCs39692941;     QzCieEsiCs39692941 = QzCieEsiCs6133225;     QzCieEsiCs6133225 = QzCieEsiCs36565413;     QzCieEsiCs36565413 = QzCieEsiCs92386898;     QzCieEsiCs92386898 = QzCieEsiCs57538559;     QzCieEsiCs57538559 = QzCieEsiCs133818;     QzCieEsiCs133818 = QzCieEsiCs43159832;     QzCieEsiCs43159832 = QzCieEsiCs44494734;     QzCieEsiCs44494734 = QzCieEsiCs28271704;     QzCieEsiCs28271704 = QzCieEsiCs15097090;     QzCieEsiCs15097090 = QzCieEsiCs4104524;     QzCieEsiCs4104524 = QzCieEsiCs48949212;     QzCieEsiCs48949212 = QzCieEsiCs99403512;     QzCieEsiCs99403512 = QzCieEsiCs23495076;     QzCieEsiCs23495076 = QzCieEsiCs24548781;     QzCieEsiCs24548781 = QzCieEsiCs14716744;     QzCieEsiCs14716744 = QzCieEsiCs31766992;     QzCieEsiCs31766992 = QzCieEsiCs95657730;     QzCieEsiCs95657730 = QzCieEsiCs51957736;     QzCieEsiCs51957736 = QzCieEsiCs65045481;     QzCieEsiCs65045481 = QzCieEsiCs89932869;     QzCieEsiCs89932869 = QzCieEsiCs89858223;     QzCieEsiCs89858223 = QzCieEsiCs86868216;     QzCieEsiCs86868216 = QzCieEsiCs14563471;     QzCieEsiCs14563471 = QzCieEsiCs3828339;     QzCieEsiCs3828339 = QzCieEsiCs44171647;     QzCieEsiCs44171647 = QzCieEsiCs84058991;     QzCieEsiCs84058991 = QzCieEsiCs15437398;     QzCieEsiCs15437398 = QzCieEsiCs90575435;     QzCieEsiCs90575435 = QzCieEsiCs47622756;     QzCieEsiCs47622756 = QzCieEsiCs78522703;     QzCieEsiCs78522703 = QzCieEsiCs24924631;     QzCieEsiCs24924631 = QzCieEsiCs61006020;     QzCieEsiCs61006020 = QzCieEsiCs73941;     QzCieEsiCs73941 = QzCieEsiCs78883368;     QzCieEsiCs78883368 = QzCieEsiCs35230127;     QzCieEsiCs35230127 = QzCieEsiCs84048615;     QzCieEsiCs84048615 = QzCieEsiCs17440721;     QzCieEsiCs17440721 = QzCieEsiCs61229911;     QzCieEsiCs61229911 = QzCieEsiCs31076072;     QzCieEsiCs31076072 = QzCieEsiCs25639286;     QzCieEsiCs25639286 = QzCieEsiCs54226800;     QzCieEsiCs54226800 = QzCieEsiCs77525135;     QzCieEsiCs77525135 = QzCieEsiCs41705915;     QzCieEsiCs41705915 = QzCieEsiCs6941385;     QzCieEsiCs6941385 = QzCieEsiCs89245161;     QzCieEsiCs89245161 = QzCieEsiCs55311400;     QzCieEsiCs55311400 = QzCieEsiCs56627775;     QzCieEsiCs56627775 = QzCieEsiCs456578;     QzCieEsiCs456578 = QzCieEsiCs95562128;     QzCieEsiCs95562128 = QzCieEsiCs80238919;     QzCieEsiCs80238919 = QzCieEsiCs91851462;     QzCieEsiCs91851462 = QzCieEsiCs29549115;     QzCieEsiCs29549115 = QzCieEsiCs1760489;     QzCieEsiCs1760489 = QzCieEsiCs48448750;     QzCieEsiCs48448750 = QzCieEsiCs49834718;     QzCieEsiCs49834718 = QzCieEsiCs19265008;     QzCieEsiCs19265008 = QzCieEsiCs22001942;     QzCieEsiCs22001942 = QzCieEsiCs88558560;     QzCieEsiCs88558560 = QzCieEsiCs13366913;     QzCieEsiCs13366913 = QzCieEsiCs16074826;     QzCieEsiCs16074826 = QzCieEsiCs27722434;     QzCieEsiCs27722434 = QzCieEsiCs53919298;     QzCieEsiCs53919298 = QzCieEsiCs80648948;     QzCieEsiCs80648948 = QzCieEsiCs36574387;     QzCieEsiCs36574387 = QzCieEsiCs79179892;     QzCieEsiCs79179892 = QzCieEsiCs87943192;     QzCieEsiCs87943192 = QzCieEsiCs99329571;     QzCieEsiCs99329571 = QzCieEsiCs44611708;     QzCieEsiCs44611708 = QzCieEsiCs89318653;     QzCieEsiCs89318653 = QzCieEsiCs30668128;     QzCieEsiCs30668128 = QzCieEsiCs14326272;     QzCieEsiCs14326272 = QzCieEsiCs34427820;     QzCieEsiCs34427820 = QzCieEsiCs20881664;     QzCieEsiCs20881664 = QzCieEsiCs39406195;     QzCieEsiCs39406195 = QzCieEsiCs35706069;     QzCieEsiCs35706069 = QzCieEsiCs12333088;     QzCieEsiCs12333088 = QzCieEsiCs45162301;     QzCieEsiCs45162301 = QzCieEsiCs7622086;     QzCieEsiCs7622086 = QzCieEsiCs14583178;     QzCieEsiCs14583178 = QzCieEsiCs41363352;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void qyLgXbnqCQ55221284() {     int yZpgpipTRW63750469 = -756941255;    int yZpgpipTRW4961529 = -646799674;    int yZpgpipTRW38596898 = 25135178;    int yZpgpipTRW33775159 = 38738848;    int yZpgpipTRW29384143 = -849834805;    int yZpgpipTRW83682234 = 92775272;    int yZpgpipTRW90877272 = -872856630;    int yZpgpipTRW81430698 = -818895808;    int yZpgpipTRW53966136 = -316318939;    int yZpgpipTRW2809494 = -423206172;    int yZpgpipTRW75736233 = -821008091;    int yZpgpipTRW10617862 = -568231881;    int yZpgpipTRW47131032 = -201013549;    int yZpgpipTRW67620111 = 98784229;    int yZpgpipTRW47014149 = -462464009;    int yZpgpipTRW56261824 = -304582172;    int yZpgpipTRW37332545 = -173897293;    int yZpgpipTRW87483267 = -386104625;    int yZpgpipTRW8087601 = 21344682;    int yZpgpipTRW16897903 = -947258041;    int yZpgpipTRW40576048 = -977165655;    int yZpgpipTRW69948442 = -888966196;    int yZpgpipTRW39858736 = 56096150;    int yZpgpipTRW18886686 = -341475694;    int yZpgpipTRW93078806 = 92274885;    int yZpgpipTRW88436812 = -718855093;    int yZpgpipTRW90249252 = -125926420;    int yZpgpipTRW50406184 = -693112493;    int yZpgpipTRW35164265 = -658359888;    int yZpgpipTRW24254270 = -780126380;    int yZpgpipTRW48395329 = -668798222;    int yZpgpipTRW6490404 = -836941580;    int yZpgpipTRW80442183 = -271165080;    int yZpgpipTRW82883763 = -510397642;    int yZpgpipTRW778146 = -794183092;    int yZpgpipTRW56409612 = -340315674;    int yZpgpipTRW25206705 = -592790202;    int yZpgpipTRW49283832 = -219659577;    int yZpgpipTRW265868 = -155129801;    int yZpgpipTRW85231195 = -587289426;    int yZpgpipTRW18690241 = -581329523;    int yZpgpipTRW78366614 = -442234409;    int yZpgpipTRW92418408 = -362782398;    int yZpgpipTRW14007590 = -826170027;    int yZpgpipTRW71811605 = -2254599;    int yZpgpipTRW52271929 = -427393105;    int yZpgpipTRW96356389 = -638525040;    int yZpgpipTRW60036072 = -904321964;    int yZpgpipTRW85035340 = -885032098;    int yZpgpipTRW44706501 = -685637483;    int yZpgpipTRW87391919 = -965692042;    int yZpgpipTRW98775888 = -160793386;    int yZpgpipTRW39320423 = -231572725;    int yZpgpipTRW60360486 = -397214279;    int yZpgpipTRW14645236 = -994610819;    int yZpgpipTRW93802026 = -867975060;    int yZpgpipTRW65102793 = -602895825;    int yZpgpipTRW19710213 = -633389129;    int yZpgpipTRW40696353 = 46463962;    int yZpgpipTRW40947331 = -30979713;    int yZpgpipTRW93432982 = -781298309;    int yZpgpipTRW40471089 = -79744137;    int yZpgpipTRW46266434 = -60535921;    int yZpgpipTRW29711867 = -536192559;    int yZpgpipTRW54414164 = -754407951;    int yZpgpipTRW69245830 = -984066512;    int yZpgpipTRW30175679 = -197066802;    int yZpgpipTRW64247268 = -690615907;    int yZpgpipTRW66841965 = -107032679;    int yZpgpipTRW90604536 = -22148336;    int yZpgpipTRW31055120 = -711791971;    int yZpgpipTRW88048713 = -954237717;    int yZpgpipTRW87217400 = -130974825;    int yZpgpipTRW22856406 = -391365892;    int yZpgpipTRW98207662 = -265928518;    int yZpgpipTRW62209433 = -434931246;    int yZpgpipTRW77530034 = -426183798;    int yZpgpipTRW25851146 = -117733824;    int yZpgpipTRW47075081 = -239221096;    int yZpgpipTRW40806878 = -480332010;    int yZpgpipTRW92080423 = 19669946;    int yZpgpipTRW30213180 = -221604456;    int yZpgpipTRW65370843 = -808080396;    int yZpgpipTRW90457763 = -972722405;    int yZpgpipTRW36862350 = -814434339;    int yZpgpipTRW49619441 = -408004837;    int yZpgpipTRW67169980 = -505368856;    int yZpgpipTRW20081697 = -873950802;    int yZpgpipTRW68238527 = -515786823;    int yZpgpipTRW6976120 = -926208033;    int yZpgpipTRW91306819 = -737419849;    int yZpgpipTRW5496492 = -959401074;    int yZpgpipTRW8587479 = -166123540;    int yZpgpipTRW59318536 = -24150089;    int yZpgpipTRW91798213 = -805991118;    int yZpgpipTRW78219151 = -401585387;    int yZpgpipTRW32100181 = -281698489;    int yZpgpipTRW62706542 = -826589839;    int yZpgpipTRW59593426 = 28237923;    int yZpgpipTRW2565776 = -756941255;     yZpgpipTRW63750469 = yZpgpipTRW4961529;     yZpgpipTRW4961529 = yZpgpipTRW38596898;     yZpgpipTRW38596898 = yZpgpipTRW33775159;     yZpgpipTRW33775159 = yZpgpipTRW29384143;     yZpgpipTRW29384143 = yZpgpipTRW83682234;     yZpgpipTRW83682234 = yZpgpipTRW90877272;     yZpgpipTRW90877272 = yZpgpipTRW81430698;     yZpgpipTRW81430698 = yZpgpipTRW53966136;     yZpgpipTRW53966136 = yZpgpipTRW2809494;     yZpgpipTRW2809494 = yZpgpipTRW75736233;     yZpgpipTRW75736233 = yZpgpipTRW10617862;     yZpgpipTRW10617862 = yZpgpipTRW47131032;     yZpgpipTRW47131032 = yZpgpipTRW67620111;     yZpgpipTRW67620111 = yZpgpipTRW47014149;     yZpgpipTRW47014149 = yZpgpipTRW56261824;     yZpgpipTRW56261824 = yZpgpipTRW37332545;     yZpgpipTRW37332545 = yZpgpipTRW87483267;     yZpgpipTRW87483267 = yZpgpipTRW8087601;     yZpgpipTRW8087601 = yZpgpipTRW16897903;     yZpgpipTRW16897903 = yZpgpipTRW40576048;     yZpgpipTRW40576048 = yZpgpipTRW69948442;     yZpgpipTRW69948442 = yZpgpipTRW39858736;     yZpgpipTRW39858736 = yZpgpipTRW18886686;     yZpgpipTRW18886686 = yZpgpipTRW93078806;     yZpgpipTRW93078806 = yZpgpipTRW88436812;     yZpgpipTRW88436812 = yZpgpipTRW90249252;     yZpgpipTRW90249252 = yZpgpipTRW50406184;     yZpgpipTRW50406184 = yZpgpipTRW35164265;     yZpgpipTRW35164265 = yZpgpipTRW24254270;     yZpgpipTRW24254270 = yZpgpipTRW48395329;     yZpgpipTRW48395329 = yZpgpipTRW6490404;     yZpgpipTRW6490404 = yZpgpipTRW80442183;     yZpgpipTRW80442183 = yZpgpipTRW82883763;     yZpgpipTRW82883763 = yZpgpipTRW778146;     yZpgpipTRW778146 = yZpgpipTRW56409612;     yZpgpipTRW56409612 = yZpgpipTRW25206705;     yZpgpipTRW25206705 = yZpgpipTRW49283832;     yZpgpipTRW49283832 = yZpgpipTRW265868;     yZpgpipTRW265868 = yZpgpipTRW85231195;     yZpgpipTRW85231195 = yZpgpipTRW18690241;     yZpgpipTRW18690241 = yZpgpipTRW78366614;     yZpgpipTRW78366614 = yZpgpipTRW92418408;     yZpgpipTRW92418408 = yZpgpipTRW14007590;     yZpgpipTRW14007590 = yZpgpipTRW71811605;     yZpgpipTRW71811605 = yZpgpipTRW52271929;     yZpgpipTRW52271929 = yZpgpipTRW96356389;     yZpgpipTRW96356389 = yZpgpipTRW60036072;     yZpgpipTRW60036072 = yZpgpipTRW85035340;     yZpgpipTRW85035340 = yZpgpipTRW44706501;     yZpgpipTRW44706501 = yZpgpipTRW87391919;     yZpgpipTRW87391919 = yZpgpipTRW98775888;     yZpgpipTRW98775888 = yZpgpipTRW39320423;     yZpgpipTRW39320423 = yZpgpipTRW60360486;     yZpgpipTRW60360486 = yZpgpipTRW14645236;     yZpgpipTRW14645236 = yZpgpipTRW93802026;     yZpgpipTRW93802026 = yZpgpipTRW65102793;     yZpgpipTRW65102793 = yZpgpipTRW19710213;     yZpgpipTRW19710213 = yZpgpipTRW40696353;     yZpgpipTRW40696353 = yZpgpipTRW40947331;     yZpgpipTRW40947331 = yZpgpipTRW93432982;     yZpgpipTRW93432982 = yZpgpipTRW40471089;     yZpgpipTRW40471089 = yZpgpipTRW46266434;     yZpgpipTRW46266434 = yZpgpipTRW29711867;     yZpgpipTRW29711867 = yZpgpipTRW54414164;     yZpgpipTRW54414164 = yZpgpipTRW69245830;     yZpgpipTRW69245830 = yZpgpipTRW30175679;     yZpgpipTRW30175679 = yZpgpipTRW64247268;     yZpgpipTRW64247268 = yZpgpipTRW66841965;     yZpgpipTRW66841965 = yZpgpipTRW90604536;     yZpgpipTRW90604536 = yZpgpipTRW31055120;     yZpgpipTRW31055120 = yZpgpipTRW88048713;     yZpgpipTRW88048713 = yZpgpipTRW87217400;     yZpgpipTRW87217400 = yZpgpipTRW22856406;     yZpgpipTRW22856406 = yZpgpipTRW98207662;     yZpgpipTRW98207662 = yZpgpipTRW62209433;     yZpgpipTRW62209433 = yZpgpipTRW77530034;     yZpgpipTRW77530034 = yZpgpipTRW25851146;     yZpgpipTRW25851146 = yZpgpipTRW47075081;     yZpgpipTRW47075081 = yZpgpipTRW40806878;     yZpgpipTRW40806878 = yZpgpipTRW92080423;     yZpgpipTRW92080423 = yZpgpipTRW30213180;     yZpgpipTRW30213180 = yZpgpipTRW65370843;     yZpgpipTRW65370843 = yZpgpipTRW90457763;     yZpgpipTRW90457763 = yZpgpipTRW36862350;     yZpgpipTRW36862350 = yZpgpipTRW49619441;     yZpgpipTRW49619441 = yZpgpipTRW67169980;     yZpgpipTRW67169980 = yZpgpipTRW20081697;     yZpgpipTRW20081697 = yZpgpipTRW68238527;     yZpgpipTRW68238527 = yZpgpipTRW6976120;     yZpgpipTRW6976120 = yZpgpipTRW91306819;     yZpgpipTRW91306819 = yZpgpipTRW5496492;     yZpgpipTRW5496492 = yZpgpipTRW8587479;     yZpgpipTRW8587479 = yZpgpipTRW59318536;     yZpgpipTRW59318536 = yZpgpipTRW91798213;     yZpgpipTRW91798213 = yZpgpipTRW78219151;     yZpgpipTRW78219151 = yZpgpipTRW32100181;     yZpgpipTRW32100181 = yZpgpipTRW62706542;     yZpgpipTRW62706542 = yZpgpipTRW59593426;     yZpgpipTRW59593426 = yZpgpipTRW2565776;     yZpgpipTRW2565776 = yZpgpipTRW63750469;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void boOcGgdpCd7898090() {     int tbEGNbSAgA670529 = -40723495;    int tbEGNbSAgA49252346 = -768035029;    int tbEGNbSAgA6621216 = -220262359;    int tbEGNbSAgA57908318 = -197193650;    int tbEGNbSAgA82323256 = -270737113;    int tbEGNbSAgA5849270 = -229776226;    int tbEGNbSAgA83059775 = -205788785;    int tbEGNbSAgA31450559 = -725479958;    int tbEGNbSAgA3036867 = -729241158;    int tbEGNbSAgA75469758 = -872269096;    int tbEGNbSAgA497115 = -414964853;    int tbEGNbSAgA83817258 = -74204356;    int tbEGNbSAgA65720709 = -19791457;    int tbEGNbSAgA99399274 = -334002433;    int tbEGNbSAgA32020046 = -639907772;    int tbEGNbSAgA85182120 = -641432671;    int tbEGNbSAgA1135911 = -248720773;    int tbEGNbSAgA97450157 = -985758775;    int tbEGNbSAgA49354239 = -19671855;    int tbEGNbSAgA41669171 = -988361992;    int tbEGNbSAgA27959073 = -399461242;    int tbEGNbSAgA53212468 = -569586069;    int tbEGNbSAgA18724675 = -781049560;    int tbEGNbSAgA56156163 = -795794600;    int tbEGNbSAgA31154464 = -313666357;    int tbEGNbSAgA11759231 = -809429920;    int tbEGNbSAgA39695076 = -453228413;    int tbEGNbSAgA59843615 = -523003508;    int tbEGNbSAgA61236669 = -961110098;    int tbEGNbSAgA97990375 = -260204293;    int tbEGNbSAgA15868751 = -132000430;    int tbEGNbSAgA7113398 = -555818722;    int tbEGNbSAgA6078372 = -630342867;    int tbEGNbSAgA12877571 = -340682844;    int tbEGNbSAgA64146604 = -130981670;    int tbEGNbSAgA76875 = -815852927;    int tbEGNbSAgA59161310 = -310608836;    int tbEGNbSAgA89091628 = -98707817;    int tbEGNbSAgA12325433 = -971151327;    int tbEGNbSAgA5986494 = -495040517;    int tbEGNbSAgA98815588 = -50238148;    int tbEGNbSAgA59631955 = -847837912;    int tbEGNbSAgA58939422 = -846045014;    int tbEGNbSAgA72360244 = -765406820;    int tbEGNbSAgA94800978 = -301461167;    int tbEGNbSAgA64945842 = -60754407;    int tbEGNbSAgA73930863 = -784549710;    int tbEGNbSAgA6137912 = -980282057;    int tbEGNbSAgA4925567 = -900716743;    int tbEGNbSAgA92317436 = -920412035;    int tbEGNbSAgA82064624 = -919685520;    int tbEGNbSAgA57325745 = -30642507;    int tbEGNbSAgA68324113 = -287505911;    int tbEGNbSAgA67468023 = -871286402;    int tbEGNbSAgA79939017 = -484948886;    int tbEGNbSAgA47458060 = -471137427;    int tbEGNbSAgA30527671 = -986985469;    int tbEGNbSAgA50465052 = -424467759;    int tbEGNbSAgA26753855 = -883527293;    int tbEGNbSAgA70564025 = -461307193;    int tbEGNbSAgA66154194 = -776547813;    int tbEGNbSAgA23216160 = -682785278;    int tbEGNbSAgA70213890 = -764369861;    int tbEGNbSAgA5046492 = -369036866;    int tbEGNbSAgA59601007 = -640268666;    int tbEGNbSAgA93383717 = -859146131;    int tbEGNbSAgA77738887 = -443861489;    int tbEGNbSAgA52843139 = -679108613;    int tbEGNbSAgA35252671 = -103020764;    int tbEGNbSAgA31943171 = -824054846;    int tbEGNbSAgA26020811 = -230823835;    int tbEGNbSAgA12044282 = -50012957;    int tbEGNbSAgA85124725 = 85392552;    int tbEGNbSAgA43367745 = -524631338;    int tbEGNbSAgA42853583 = -838123845;    int tbEGNbSAgA68327117 = -551623330;    int tbEGNbSAgA94273046 = -723541056;    int tbEGNbSAgA46364431 = 84357259;    int tbEGNbSAgA61355184 = -394333434;    int tbEGNbSAgA66208621 = -152911951;    int tbEGNbSAgA37828368 = 75119789;    int tbEGNbSAgA33557164 = -472946357;    int tbEGNbSAgA54918049 = -622286766;    int tbEGNbSAgA68919233 = 59301936;    int tbEGNbSAgA15925751 = -340518773;    int tbEGNbSAgA58543006 = -1357924;    int tbEGNbSAgA38789285 = -168312811;    int tbEGNbSAgA38610348 = -759056466;    int tbEGNbSAgA32938553 = -855733959;    int tbEGNbSAgA16688544 = -659844244;    int tbEGNbSAgA69549203 = -828867458;    int tbEGNbSAgA8696258 = -886141078;    int tbEGNbSAgA62337774 = -215180524;    int tbEGNbSAgA41761408 = -409844134;    int tbEGNbSAgA39832300 = -718492705;    int tbEGNbSAgA75599428 = -367452870;    int tbEGNbSAgA89418065 = 16531948;    int tbEGNbSAgA53892930 = -377008149;    int tbEGNbSAgA12759237 = -25138154;    int tbEGNbSAgA1417262 = -40723495;     tbEGNbSAgA670529 = tbEGNbSAgA49252346;     tbEGNbSAgA49252346 = tbEGNbSAgA6621216;     tbEGNbSAgA6621216 = tbEGNbSAgA57908318;     tbEGNbSAgA57908318 = tbEGNbSAgA82323256;     tbEGNbSAgA82323256 = tbEGNbSAgA5849270;     tbEGNbSAgA5849270 = tbEGNbSAgA83059775;     tbEGNbSAgA83059775 = tbEGNbSAgA31450559;     tbEGNbSAgA31450559 = tbEGNbSAgA3036867;     tbEGNbSAgA3036867 = tbEGNbSAgA75469758;     tbEGNbSAgA75469758 = tbEGNbSAgA497115;     tbEGNbSAgA497115 = tbEGNbSAgA83817258;     tbEGNbSAgA83817258 = tbEGNbSAgA65720709;     tbEGNbSAgA65720709 = tbEGNbSAgA99399274;     tbEGNbSAgA99399274 = tbEGNbSAgA32020046;     tbEGNbSAgA32020046 = tbEGNbSAgA85182120;     tbEGNbSAgA85182120 = tbEGNbSAgA1135911;     tbEGNbSAgA1135911 = tbEGNbSAgA97450157;     tbEGNbSAgA97450157 = tbEGNbSAgA49354239;     tbEGNbSAgA49354239 = tbEGNbSAgA41669171;     tbEGNbSAgA41669171 = tbEGNbSAgA27959073;     tbEGNbSAgA27959073 = tbEGNbSAgA53212468;     tbEGNbSAgA53212468 = tbEGNbSAgA18724675;     tbEGNbSAgA18724675 = tbEGNbSAgA56156163;     tbEGNbSAgA56156163 = tbEGNbSAgA31154464;     tbEGNbSAgA31154464 = tbEGNbSAgA11759231;     tbEGNbSAgA11759231 = tbEGNbSAgA39695076;     tbEGNbSAgA39695076 = tbEGNbSAgA59843615;     tbEGNbSAgA59843615 = tbEGNbSAgA61236669;     tbEGNbSAgA61236669 = tbEGNbSAgA97990375;     tbEGNbSAgA97990375 = tbEGNbSAgA15868751;     tbEGNbSAgA15868751 = tbEGNbSAgA7113398;     tbEGNbSAgA7113398 = tbEGNbSAgA6078372;     tbEGNbSAgA6078372 = tbEGNbSAgA12877571;     tbEGNbSAgA12877571 = tbEGNbSAgA64146604;     tbEGNbSAgA64146604 = tbEGNbSAgA76875;     tbEGNbSAgA76875 = tbEGNbSAgA59161310;     tbEGNbSAgA59161310 = tbEGNbSAgA89091628;     tbEGNbSAgA89091628 = tbEGNbSAgA12325433;     tbEGNbSAgA12325433 = tbEGNbSAgA5986494;     tbEGNbSAgA5986494 = tbEGNbSAgA98815588;     tbEGNbSAgA98815588 = tbEGNbSAgA59631955;     tbEGNbSAgA59631955 = tbEGNbSAgA58939422;     tbEGNbSAgA58939422 = tbEGNbSAgA72360244;     tbEGNbSAgA72360244 = tbEGNbSAgA94800978;     tbEGNbSAgA94800978 = tbEGNbSAgA64945842;     tbEGNbSAgA64945842 = tbEGNbSAgA73930863;     tbEGNbSAgA73930863 = tbEGNbSAgA6137912;     tbEGNbSAgA6137912 = tbEGNbSAgA4925567;     tbEGNbSAgA4925567 = tbEGNbSAgA92317436;     tbEGNbSAgA92317436 = tbEGNbSAgA82064624;     tbEGNbSAgA82064624 = tbEGNbSAgA57325745;     tbEGNbSAgA57325745 = tbEGNbSAgA68324113;     tbEGNbSAgA68324113 = tbEGNbSAgA67468023;     tbEGNbSAgA67468023 = tbEGNbSAgA79939017;     tbEGNbSAgA79939017 = tbEGNbSAgA47458060;     tbEGNbSAgA47458060 = tbEGNbSAgA30527671;     tbEGNbSAgA30527671 = tbEGNbSAgA50465052;     tbEGNbSAgA50465052 = tbEGNbSAgA26753855;     tbEGNbSAgA26753855 = tbEGNbSAgA70564025;     tbEGNbSAgA70564025 = tbEGNbSAgA66154194;     tbEGNbSAgA66154194 = tbEGNbSAgA23216160;     tbEGNbSAgA23216160 = tbEGNbSAgA70213890;     tbEGNbSAgA70213890 = tbEGNbSAgA5046492;     tbEGNbSAgA5046492 = tbEGNbSAgA59601007;     tbEGNbSAgA59601007 = tbEGNbSAgA93383717;     tbEGNbSAgA93383717 = tbEGNbSAgA77738887;     tbEGNbSAgA77738887 = tbEGNbSAgA52843139;     tbEGNbSAgA52843139 = tbEGNbSAgA35252671;     tbEGNbSAgA35252671 = tbEGNbSAgA31943171;     tbEGNbSAgA31943171 = tbEGNbSAgA26020811;     tbEGNbSAgA26020811 = tbEGNbSAgA12044282;     tbEGNbSAgA12044282 = tbEGNbSAgA85124725;     tbEGNbSAgA85124725 = tbEGNbSAgA43367745;     tbEGNbSAgA43367745 = tbEGNbSAgA42853583;     tbEGNbSAgA42853583 = tbEGNbSAgA68327117;     tbEGNbSAgA68327117 = tbEGNbSAgA94273046;     tbEGNbSAgA94273046 = tbEGNbSAgA46364431;     tbEGNbSAgA46364431 = tbEGNbSAgA61355184;     tbEGNbSAgA61355184 = tbEGNbSAgA66208621;     tbEGNbSAgA66208621 = tbEGNbSAgA37828368;     tbEGNbSAgA37828368 = tbEGNbSAgA33557164;     tbEGNbSAgA33557164 = tbEGNbSAgA54918049;     tbEGNbSAgA54918049 = tbEGNbSAgA68919233;     tbEGNbSAgA68919233 = tbEGNbSAgA15925751;     tbEGNbSAgA15925751 = tbEGNbSAgA58543006;     tbEGNbSAgA58543006 = tbEGNbSAgA38789285;     tbEGNbSAgA38789285 = tbEGNbSAgA38610348;     tbEGNbSAgA38610348 = tbEGNbSAgA32938553;     tbEGNbSAgA32938553 = tbEGNbSAgA16688544;     tbEGNbSAgA16688544 = tbEGNbSAgA69549203;     tbEGNbSAgA69549203 = tbEGNbSAgA8696258;     tbEGNbSAgA8696258 = tbEGNbSAgA62337774;     tbEGNbSAgA62337774 = tbEGNbSAgA41761408;     tbEGNbSAgA41761408 = tbEGNbSAgA39832300;     tbEGNbSAgA39832300 = tbEGNbSAgA75599428;     tbEGNbSAgA75599428 = tbEGNbSAgA89418065;     tbEGNbSAgA89418065 = tbEGNbSAgA53892930;     tbEGNbSAgA53892930 = tbEGNbSAgA12759237;     tbEGNbSAgA12759237 = tbEGNbSAgA1417262;     tbEGNbSAgA1417262 = tbEGNbSAgA670529;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void yULuAjPeUD44543065() {     int XqHLEdoXCV23057646 = -775127458;    int XqHLEdoXCV33599847 = -716578084;    int XqHLEdoXCV35390494 = -459916203;    int XqHLEdoXCV72915007 = -203601182;    int XqHLEdoXCV80497510 = -861794114;    int XqHLEdoXCV20732387 = -90384604;    int XqHLEdoXCV75215514 = -27776091;    int XqHLEdoXCV7084418 = -96076602;    int XqHLEdoXCV199998 = -613437197;    int XqHLEdoXCV67233342 = -615010429;    int XqHLEdoXCV38038975 = -411589254;    int XqHLEdoXCV39720208 = -324949650;    int XqHLEdoXCV32728890 = -638508776;    int XqHLEdoXCV42014027 = -861711190;    int XqHLEdoXCV68755323 = -267385485;    int XqHLEdoXCV29438033 = 76899787;    int XqHLEdoXCV50959262 = -526000593;    int XqHLEdoXCV3426575 = -337501465;    int XqHLEdoXCV90635870 = -154773980;    int XqHLEdoXCV20185455 = -140996446;    int XqHLEdoXCV28842180 = -390948550;    int XqHLEdoXCV17027687 = 25647618;    int XqHLEdoXCV22017998 = -957130152;    int XqHLEdoXCV82655950 = -259787457;    int XqHLEdoXCV66694710 = 64774518;    int XqHLEdoXCV62226 = -153324144;    int XqHLEdoXCV86784496 = -210814906;    int XqHLEdoXCV65755065 = -311457962;    int XqHLEdoXCV68129230 = -205367217;    int XqHLEdoXCV7147555 = -180500160;    int XqHLEdoXCV60159556 = -213316582;    int XqHLEdoXCV64654588 = -107890076;    int XqHLEdoXCV87117042 = -59686362;    int XqHLEdoXCV72266258 = -664809694;    int XqHLEdoXCV40375969 = -194738092;    int XqHLEdoXCV41769744 = -31248303;    int XqHLEdoXCV52601022 = -43556788;    int XqHLEdoXCV42717729 = 33666711;    int XqHLEdoXCV60633564 = -272197359;    int XqHLEdoXCV26172209 = -631264546;    int XqHLEdoXCV27572959 = -553415814;    int XqHLEdoXCV48140346 = -605746603;    int XqHLEdoXCV64489614 = -733575598;    int XqHLEdoXCV71804363 = -762770469;    int XqHLEdoXCV62784245 = -110604645;    int XqHLEdoXCV73046124 = -242983503;    int XqHLEdoXCV86228260 = -262959962;    int XqHLEdoXCV50736586 = -736817048;    int XqHLEdoXCV99385471 = -850433441;    int XqHLEdoXCV89401182 = 37567706;    int XqHLEdoXCV90933841 = -174761797;    int XqHLEdoXCV31177002 = -461461789;    int XqHLEdoXCV46638516 = 40582859;    int XqHLEdoXCV27754569 = -46967210;    int XqHLEdoXCV15700885 = -625653001;    int XqHLEdoXCV6029960 = -700775077;    int XqHLEdoXCV11581849 = -759447932;    int XqHLEdoXCV52734544 = -100128746;    int XqHLEdoXCV6220297 = -168375700;    int XqHLEdoXCV80435284 = -608469970;    int XqHLEdoXCV33947891 = -879569698;    int XqHLEdoXCV9460449 = -716318130;    int XqHLEdoXCV38955188 = -890709386;    int XqHLEdoXCV93052442 = -332937038;    int XqHLEdoXCV7073787 = -301693847;    int XqHLEdoXCV73384386 = -203699178;    int XqHLEdoXCV52603166 = -165263288;    int XqHLEdoXCV60462632 = -973699082;    int XqHLEdoXCV1638059 = -566973098;    int XqHLEdoXCV26985579 = -136137183;    int XqHLEdoXCV76837010 = -879543426;    int XqHLEdoXCV8241533 = -459667305;    int XqHLEdoXCV42793010 = 34695893;    int XqHLEdoXCV64463661 = -523509434;    int XqHLEdoXCV92612495 = -587580633;    int XqHLEdoXCV80701833 = -785201948;    int XqHLEdoXCV52538073 = -240776785;    int XqHLEdoXCV50213635 = -94359684;    int XqHLEdoXCV19871705 = -49182813;    int XqHLEdoXCV93648586 = -692241980;    int XqHLEdoXCV13833965 = -890364182;    int XqHLEdoXCV36047910 = -473997859;    int XqHLEdoXCV66369594 = -461024521;    int XqHLEdoXCV78728048 = -142934924;    int XqHLEdoXCV16213714 = 94261637;    int XqHLEdoXCV28982554 = -751854794;    int XqHLEdoXCV18016073 = -48472936;    int XqHLEdoXCV59362473 = 87280848;    int XqHLEdoXCV56565373 = 60843306;    int XqHLEdoXCV34346009 = -493963016;    int XqHLEdoXCV30187896 = -271800372;    int XqHLEdoXCV99866478 = -943428042;    int XqHLEdoXCV36497433 = -797957589;    int XqHLEdoXCV80198279 = -663727389;    int XqHLEdoXCV92224317 = -751694849;    int XqHLEdoXCV18112510 = -837097684;    int XqHLEdoXCV9185159 = -715037218;    int XqHLEdoXCV71437171 = -300638560;    int XqHLEdoXCV64730577 = -361076622;    int XqHLEdoXCV89399859 = -775127458;     XqHLEdoXCV23057646 = XqHLEdoXCV33599847;     XqHLEdoXCV33599847 = XqHLEdoXCV35390494;     XqHLEdoXCV35390494 = XqHLEdoXCV72915007;     XqHLEdoXCV72915007 = XqHLEdoXCV80497510;     XqHLEdoXCV80497510 = XqHLEdoXCV20732387;     XqHLEdoXCV20732387 = XqHLEdoXCV75215514;     XqHLEdoXCV75215514 = XqHLEdoXCV7084418;     XqHLEdoXCV7084418 = XqHLEdoXCV199998;     XqHLEdoXCV199998 = XqHLEdoXCV67233342;     XqHLEdoXCV67233342 = XqHLEdoXCV38038975;     XqHLEdoXCV38038975 = XqHLEdoXCV39720208;     XqHLEdoXCV39720208 = XqHLEdoXCV32728890;     XqHLEdoXCV32728890 = XqHLEdoXCV42014027;     XqHLEdoXCV42014027 = XqHLEdoXCV68755323;     XqHLEdoXCV68755323 = XqHLEdoXCV29438033;     XqHLEdoXCV29438033 = XqHLEdoXCV50959262;     XqHLEdoXCV50959262 = XqHLEdoXCV3426575;     XqHLEdoXCV3426575 = XqHLEdoXCV90635870;     XqHLEdoXCV90635870 = XqHLEdoXCV20185455;     XqHLEdoXCV20185455 = XqHLEdoXCV28842180;     XqHLEdoXCV28842180 = XqHLEdoXCV17027687;     XqHLEdoXCV17027687 = XqHLEdoXCV22017998;     XqHLEdoXCV22017998 = XqHLEdoXCV82655950;     XqHLEdoXCV82655950 = XqHLEdoXCV66694710;     XqHLEdoXCV66694710 = XqHLEdoXCV62226;     XqHLEdoXCV62226 = XqHLEdoXCV86784496;     XqHLEdoXCV86784496 = XqHLEdoXCV65755065;     XqHLEdoXCV65755065 = XqHLEdoXCV68129230;     XqHLEdoXCV68129230 = XqHLEdoXCV7147555;     XqHLEdoXCV7147555 = XqHLEdoXCV60159556;     XqHLEdoXCV60159556 = XqHLEdoXCV64654588;     XqHLEdoXCV64654588 = XqHLEdoXCV87117042;     XqHLEdoXCV87117042 = XqHLEdoXCV72266258;     XqHLEdoXCV72266258 = XqHLEdoXCV40375969;     XqHLEdoXCV40375969 = XqHLEdoXCV41769744;     XqHLEdoXCV41769744 = XqHLEdoXCV52601022;     XqHLEdoXCV52601022 = XqHLEdoXCV42717729;     XqHLEdoXCV42717729 = XqHLEdoXCV60633564;     XqHLEdoXCV60633564 = XqHLEdoXCV26172209;     XqHLEdoXCV26172209 = XqHLEdoXCV27572959;     XqHLEdoXCV27572959 = XqHLEdoXCV48140346;     XqHLEdoXCV48140346 = XqHLEdoXCV64489614;     XqHLEdoXCV64489614 = XqHLEdoXCV71804363;     XqHLEdoXCV71804363 = XqHLEdoXCV62784245;     XqHLEdoXCV62784245 = XqHLEdoXCV73046124;     XqHLEdoXCV73046124 = XqHLEdoXCV86228260;     XqHLEdoXCV86228260 = XqHLEdoXCV50736586;     XqHLEdoXCV50736586 = XqHLEdoXCV99385471;     XqHLEdoXCV99385471 = XqHLEdoXCV89401182;     XqHLEdoXCV89401182 = XqHLEdoXCV90933841;     XqHLEdoXCV90933841 = XqHLEdoXCV31177002;     XqHLEdoXCV31177002 = XqHLEdoXCV46638516;     XqHLEdoXCV46638516 = XqHLEdoXCV27754569;     XqHLEdoXCV27754569 = XqHLEdoXCV15700885;     XqHLEdoXCV15700885 = XqHLEdoXCV6029960;     XqHLEdoXCV6029960 = XqHLEdoXCV11581849;     XqHLEdoXCV11581849 = XqHLEdoXCV52734544;     XqHLEdoXCV52734544 = XqHLEdoXCV6220297;     XqHLEdoXCV6220297 = XqHLEdoXCV80435284;     XqHLEdoXCV80435284 = XqHLEdoXCV33947891;     XqHLEdoXCV33947891 = XqHLEdoXCV9460449;     XqHLEdoXCV9460449 = XqHLEdoXCV38955188;     XqHLEdoXCV38955188 = XqHLEdoXCV93052442;     XqHLEdoXCV93052442 = XqHLEdoXCV7073787;     XqHLEdoXCV7073787 = XqHLEdoXCV73384386;     XqHLEdoXCV73384386 = XqHLEdoXCV52603166;     XqHLEdoXCV52603166 = XqHLEdoXCV60462632;     XqHLEdoXCV60462632 = XqHLEdoXCV1638059;     XqHLEdoXCV1638059 = XqHLEdoXCV26985579;     XqHLEdoXCV26985579 = XqHLEdoXCV76837010;     XqHLEdoXCV76837010 = XqHLEdoXCV8241533;     XqHLEdoXCV8241533 = XqHLEdoXCV42793010;     XqHLEdoXCV42793010 = XqHLEdoXCV64463661;     XqHLEdoXCV64463661 = XqHLEdoXCV92612495;     XqHLEdoXCV92612495 = XqHLEdoXCV80701833;     XqHLEdoXCV80701833 = XqHLEdoXCV52538073;     XqHLEdoXCV52538073 = XqHLEdoXCV50213635;     XqHLEdoXCV50213635 = XqHLEdoXCV19871705;     XqHLEdoXCV19871705 = XqHLEdoXCV93648586;     XqHLEdoXCV93648586 = XqHLEdoXCV13833965;     XqHLEdoXCV13833965 = XqHLEdoXCV36047910;     XqHLEdoXCV36047910 = XqHLEdoXCV66369594;     XqHLEdoXCV66369594 = XqHLEdoXCV78728048;     XqHLEdoXCV78728048 = XqHLEdoXCV16213714;     XqHLEdoXCV16213714 = XqHLEdoXCV28982554;     XqHLEdoXCV28982554 = XqHLEdoXCV18016073;     XqHLEdoXCV18016073 = XqHLEdoXCV59362473;     XqHLEdoXCV59362473 = XqHLEdoXCV56565373;     XqHLEdoXCV56565373 = XqHLEdoXCV34346009;     XqHLEdoXCV34346009 = XqHLEdoXCV30187896;     XqHLEdoXCV30187896 = XqHLEdoXCV99866478;     XqHLEdoXCV99866478 = XqHLEdoXCV36497433;     XqHLEdoXCV36497433 = XqHLEdoXCV80198279;     XqHLEdoXCV80198279 = XqHLEdoXCV92224317;     XqHLEdoXCV92224317 = XqHLEdoXCV18112510;     XqHLEdoXCV18112510 = XqHLEdoXCV9185159;     XqHLEdoXCV9185159 = XqHLEdoXCV71437171;     XqHLEdoXCV71437171 = XqHLEdoXCV64730577;     XqHLEdoXCV64730577 = XqHLEdoXCV89399859;     XqHLEdoXCV89399859 = XqHLEdoXCV23057646;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void QIKPklpQwu81188039() {     int eSUIgzeUgJ45444763 = -409531421;    int eSUIgzeUgJ17947347 = -665121138;    int eSUIgzeUgJ64159772 = -699570047;    int eSUIgzeUgJ87921695 = -210008714;    int eSUIgzeUgJ78671764 = -352851115;    int eSUIgzeUgJ35615504 = 49007019;    int eSUIgzeUgJ67371253 = -949763398;    int eSUIgzeUgJ82718276 = -566673246;    int eSUIgzeUgJ97363129 = -497633237;    int eSUIgzeUgJ58996927 = -357751762;    int eSUIgzeUgJ75580834 = -408213654;    int eSUIgzeUgJ95623156 = -575694943;    int eSUIgzeUgJ99737070 = -157226096;    int eSUIgzeUgJ84628779 = -289419947;    int eSUIgzeUgJ5490600 = -994863199;    int eSUIgzeUgJ73693945 = -304767756;    int eSUIgzeUgJ782614 = -803280413;    int eSUIgzeUgJ9402991 = -789244156;    int eSUIgzeUgJ31917501 = -289876105;    int eSUIgzeUgJ98701737 = -393630900;    int eSUIgzeUgJ29725287 = -382435858;    int eSUIgzeUgJ80842905 = -479118696;    int eSUIgzeUgJ25311321 = -33210744;    int eSUIgzeUgJ9155738 = -823780315;    int eSUIgzeUgJ2234958 = -656784606;    int eSUIgzeUgJ88365220 = -597218368;    int eSUIgzeUgJ33873916 = 31598601;    int eSUIgzeUgJ71666515 = -99912415;    int eSUIgzeUgJ75021791 = -549624336;    int eSUIgzeUgJ16304735 = -100796026;    int eSUIgzeUgJ4450362 = -294632734;    int eSUIgzeUgJ22195780 = -759961431;    int eSUIgzeUgJ68155712 = -589029857;    int eSUIgzeUgJ31654946 = -988936544;    int eSUIgzeUgJ16605333 = -258494515;    int eSUIgzeUgJ83462613 = -346643680;    int eSUIgzeUgJ46040734 = -876504739;    int eSUIgzeUgJ96343830 = -933958760;    int eSUIgzeUgJ8941696 = -673243390;    int eSUIgzeUgJ46357923 = -767488576;    int eSUIgzeUgJ56330330 = 43406520;    int eSUIgzeUgJ36648738 = -363655294;    int eSUIgzeUgJ70039806 = -621106181;    int eSUIgzeUgJ71248482 = -760134118;    int eSUIgzeUgJ30767512 = 80251878;    int eSUIgzeUgJ81146406 = -425212598;    int eSUIgzeUgJ98525658 = -841370215;    int eSUIgzeUgJ95335260 = -493352039;    int eSUIgzeUgJ93845376 = -800150140;    int eSUIgzeUgJ86484928 = -104452553;    int eSUIgzeUgJ99803057 = -529838074;    int eSUIgzeUgJ5028260 = -892281071;    int eSUIgzeUgJ24952919 = -731328371;    int eSUIgzeUgJ88041113 = -322648019;    int eSUIgzeUgJ51462753 = -766357116;    int eSUIgzeUgJ64601858 = -930412726;    int eSUIgzeUgJ92636025 = -531910394;    int eSUIgzeUgJ55004035 = -875789733;    int eSUIgzeUgJ85686738 = -553224108;    int eSUIgzeUgJ90306544 = -755632748;    int eSUIgzeUgJ1741588 = -982591583;    int eSUIgzeUgJ95704737 = -749850983;    int eSUIgzeUgJ7696486 = 82951089;    int eSUIgzeUgJ81058394 = -296837211;    int eSUIgzeUgJ54546566 = 36880972;    int eSUIgzeUgJ53385055 = -648252224;    int eSUIgzeUgJ27467445 = -986665086;    int eSUIgzeUgJ68082125 = -168289552;    int eSUIgzeUgJ68023446 = 69074567;    int eSUIgzeUgJ22027987 = -548219519;    int eSUIgzeUgJ27653211 = -428263017;    int eSUIgzeUgJ4438784 = -869321653;    int eSUIgzeUgJ461296 = -16000766;    int eSUIgzeUgJ85559578 = -522387530;    int eSUIgzeUgJ42371408 = -337037420;    int eSUIgzeUgJ93076549 = 81219435;    int eSUIgzeUgJ10803100 = -858012515;    int eSUIgzeUgJ54062839 = -273076627;    int eSUIgzeUgJ78388225 = -804032193;    int eSUIgzeUgJ21088552 = -131572009;    int eSUIgzeUgJ89839562 = -755848153;    int eSUIgzeUgJ38538656 = -475049361;    int eSUIgzeUgJ77821139 = -299762276;    int eSUIgzeUgJ88536863 = -345171784;    int eSUIgzeUgJ16501677 = -570957952;    int eSUIgzeUgJ99422102 = -402351664;    int eSUIgzeUgJ97242860 = 71366940;    int eSUIgzeUgJ80114598 = -166381838;    int eSUIgzeUgJ80192193 = -122579429;    int eSUIgzeUgJ52003475 = -328081789;    int eSUIgzeUgJ90826587 = -814733286;    int eSUIgzeUgJ91036699 = 99284993;    int eSUIgzeUgJ10657093 = -280734653;    int eSUIgzeUgJ18635151 = -917610643;    int eSUIgzeUgJ44616336 = -784896993;    int eSUIgzeUgJ60625592 = -206742497;    int eSUIgzeUgJ28952252 = -346606384;    int eSUIgzeUgJ88981411 = -224268971;    int eSUIgzeUgJ16701917 = -697015090;    int eSUIgzeUgJ77382457 = -409531421;     eSUIgzeUgJ45444763 = eSUIgzeUgJ17947347;     eSUIgzeUgJ17947347 = eSUIgzeUgJ64159772;     eSUIgzeUgJ64159772 = eSUIgzeUgJ87921695;     eSUIgzeUgJ87921695 = eSUIgzeUgJ78671764;     eSUIgzeUgJ78671764 = eSUIgzeUgJ35615504;     eSUIgzeUgJ35615504 = eSUIgzeUgJ67371253;     eSUIgzeUgJ67371253 = eSUIgzeUgJ82718276;     eSUIgzeUgJ82718276 = eSUIgzeUgJ97363129;     eSUIgzeUgJ97363129 = eSUIgzeUgJ58996927;     eSUIgzeUgJ58996927 = eSUIgzeUgJ75580834;     eSUIgzeUgJ75580834 = eSUIgzeUgJ95623156;     eSUIgzeUgJ95623156 = eSUIgzeUgJ99737070;     eSUIgzeUgJ99737070 = eSUIgzeUgJ84628779;     eSUIgzeUgJ84628779 = eSUIgzeUgJ5490600;     eSUIgzeUgJ5490600 = eSUIgzeUgJ73693945;     eSUIgzeUgJ73693945 = eSUIgzeUgJ782614;     eSUIgzeUgJ782614 = eSUIgzeUgJ9402991;     eSUIgzeUgJ9402991 = eSUIgzeUgJ31917501;     eSUIgzeUgJ31917501 = eSUIgzeUgJ98701737;     eSUIgzeUgJ98701737 = eSUIgzeUgJ29725287;     eSUIgzeUgJ29725287 = eSUIgzeUgJ80842905;     eSUIgzeUgJ80842905 = eSUIgzeUgJ25311321;     eSUIgzeUgJ25311321 = eSUIgzeUgJ9155738;     eSUIgzeUgJ9155738 = eSUIgzeUgJ2234958;     eSUIgzeUgJ2234958 = eSUIgzeUgJ88365220;     eSUIgzeUgJ88365220 = eSUIgzeUgJ33873916;     eSUIgzeUgJ33873916 = eSUIgzeUgJ71666515;     eSUIgzeUgJ71666515 = eSUIgzeUgJ75021791;     eSUIgzeUgJ75021791 = eSUIgzeUgJ16304735;     eSUIgzeUgJ16304735 = eSUIgzeUgJ4450362;     eSUIgzeUgJ4450362 = eSUIgzeUgJ22195780;     eSUIgzeUgJ22195780 = eSUIgzeUgJ68155712;     eSUIgzeUgJ68155712 = eSUIgzeUgJ31654946;     eSUIgzeUgJ31654946 = eSUIgzeUgJ16605333;     eSUIgzeUgJ16605333 = eSUIgzeUgJ83462613;     eSUIgzeUgJ83462613 = eSUIgzeUgJ46040734;     eSUIgzeUgJ46040734 = eSUIgzeUgJ96343830;     eSUIgzeUgJ96343830 = eSUIgzeUgJ8941696;     eSUIgzeUgJ8941696 = eSUIgzeUgJ46357923;     eSUIgzeUgJ46357923 = eSUIgzeUgJ56330330;     eSUIgzeUgJ56330330 = eSUIgzeUgJ36648738;     eSUIgzeUgJ36648738 = eSUIgzeUgJ70039806;     eSUIgzeUgJ70039806 = eSUIgzeUgJ71248482;     eSUIgzeUgJ71248482 = eSUIgzeUgJ30767512;     eSUIgzeUgJ30767512 = eSUIgzeUgJ81146406;     eSUIgzeUgJ81146406 = eSUIgzeUgJ98525658;     eSUIgzeUgJ98525658 = eSUIgzeUgJ95335260;     eSUIgzeUgJ95335260 = eSUIgzeUgJ93845376;     eSUIgzeUgJ93845376 = eSUIgzeUgJ86484928;     eSUIgzeUgJ86484928 = eSUIgzeUgJ99803057;     eSUIgzeUgJ99803057 = eSUIgzeUgJ5028260;     eSUIgzeUgJ5028260 = eSUIgzeUgJ24952919;     eSUIgzeUgJ24952919 = eSUIgzeUgJ88041113;     eSUIgzeUgJ88041113 = eSUIgzeUgJ51462753;     eSUIgzeUgJ51462753 = eSUIgzeUgJ64601858;     eSUIgzeUgJ64601858 = eSUIgzeUgJ92636025;     eSUIgzeUgJ92636025 = eSUIgzeUgJ55004035;     eSUIgzeUgJ55004035 = eSUIgzeUgJ85686738;     eSUIgzeUgJ85686738 = eSUIgzeUgJ90306544;     eSUIgzeUgJ90306544 = eSUIgzeUgJ1741588;     eSUIgzeUgJ1741588 = eSUIgzeUgJ95704737;     eSUIgzeUgJ95704737 = eSUIgzeUgJ7696486;     eSUIgzeUgJ7696486 = eSUIgzeUgJ81058394;     eSUIgzeUgJ81058394 = eSUIgzeUgJ54546566;     eSUIgzeUgJ54546566 = eSUIgzeUgJ53385055;     eSUIgzeUgJ53385055 = eSUIgzeUgJ27467445;     eSUIgzeUgJ27467445 = eSUIgzeUgJ68082125;     eSUIgzeUgJ68082125 = eSUIgzeUgJ68023446;     eSUIgzeUgJ68023446 = eSUIgzeUgJ22027987;     eSUIgzeUgJ22027987 = eSUIgzeUgJ27653211;     eSUIgzeUgJ27653211 = eSUIgzeUgJ4438784;     eSUIgzeUgJ4438784 = eSUIgzeUgJ461296;     eSUIgzeUgJ461296 = eSUIgzeUgJ85559578;     eSUIgzeUgJ85559578 = eSUIgzeUgJ42371408;     eSUIgzeUgJ42371408 = eSUIgzeUgJ93076549;     eSUIgzeUgJ93076549 = eSUIgzeUgJ10803100;     eSUIgzeUgJ10803100 = eSUIgzeUgJ54062839;     eSUIgzeUgJ54062839 = eSUIgzeUgJ78388225;     eSUIgzeUgJ78388225 = eSUIgzeUgJ21088552;     eSUIgzeUgJ21088552 = eSUIgzeUgJ89839562;     eSUIgzeUgJ89839562 = eSUIgzeUgJ38538656;     eSUIgzeUgJ38538656 = eSUIgzeUgJ77821139;     eSUIgzeUgJ77821139 = eSUIgzeUgJ88536863;     eSUIgzeUgJ88536863 = eSUIgzeUgJ16501677;     eSUIgzeUgJ16501677 = eSUIgzeUgJ99422102;     eSUIgzeUgJ99422102 = eSUIgzeUgJ97242860;     eSUIgzeUgJ97242860 = eSUIgzeUgJ80114598;     eSUIgzeUgJ80114598 = eSUIgzeUgJ80192193;     eSUIgzeUgJ80192193 = eSUIgzeUgJ52003475;     eSUIgzeUgJ52003475 = eSUIgzeUgJ90826587;     eSUIgzeUgJ90826587 = eSUIgzeUgJ91036699;     eSUIgzeUgJ91036699 = eSUIgzeUgJ10657093;     eSUIgzeUgJ10657093 = eSUIgzeUgJ18635151;     eSUIgzeUgJ18635151 = eSUIgzeUgJ44616336;     eSUIgzeUgJ44616336 = eSUIgzeUgJ60625592;     eSUIgzeUgJ60625592 = eSUIgzeUgJ28952252;     eSUIgzeUgJ28952252 = eSUIgzeUgJ88981411;     eSUIgzeUgJ88981411 = eSUIgzeUgJ16701917;     eSUIgzeUgJ16701917 = eSUIgzeUgJ77382457;     eSUIgzeUgJ77382457 = eSUIgzeUgJ45444763;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void MSkLKgcGKh17833014() {     int acvTcvzUVc67831881 = -43935385;    int acvTcvzUVc2294848 = -613664193;    int acvTcvzUVc92929051 = -939223891;    int acvTcvzUVc2928384 = -216416245;    int acvTcvzUVc76846018 = -943908117;    int acvTcvzUVc50498621 = -911601359;    int acvTcvzUVc59526992 = -771750704;    int acvTcvzUVc58352135 = 62730109;    int acvTcvzUVc94526260 = -381829276;    int acvTcvzUVc50760512 = -100493095;    int acvTcvzUVc13122694 = -404838055;    int acvTcvzUVc51526106 = -826440236;    int acvTcvzUVc66745251 = -775943416;    int acvTcvzUVc27243532 = -817128704;    int acvTcvzUVc42225877 = -622340912;    int acvTcvzUVc17949858 = -686435298;    int acvTcvzUVc50605965 = 19439768;    int acvTcvzUVc15379407 = -140986847;    int acvTcvzUVc73199132 = -424978230;    int acvTcvzUVc77218021 = -646265353;    int acvTcvzUVc30608394 = -373923167;    int acvTcvzUVc44658124 = -983885009;    int acvTcvzUVc28604644 = -209291336;    int acvTcvzUVc35655525 = -287773172;    int acvTcvzUVc37775204 = -278343731;    int acvTcvzUVc76668214 = 58887408;    int acvTcvzUVc80963336 = -825987892;    int acvTcvzUVc77577965 = -988366869;    int acvTcvzUVc81914352 = -893881455;    int acvTcvzUVc25461914 = -21091893;    int acvTcvzUVc48741167 = -375948886;    int acvTcvzUVc79736970 = -312032786;    int acvTcvzUVc49194383 = -18373352;    int acvTcvzUVc91043632 = -213063395;    int acvTcvzUVc92834697 = -322250937;    int acvTcvzUVc25155482 = -662039056;    int acvTcvzUVc39480446 = -609452691;    int acvTcvzUVc49969931 = -801584232;    int acvTcvzUVc57249826 = 25710578;    int acvTcvzUVc66543637 = -903712605;    int acvTcvzUVc85087700 = -459771146;    int acvTcvzUVc25157129 = -121563985;    int acvTcvzUVc75589998 = -508636765;    int acvTcvzUVc70692601 = -757497767;    int acvTcvzUVc98750778 = -828891600;    int acvTcvzUVc89246688 = -607441693;    int acvTcvzUVc10823056 = -319780468;    int acvTcvzUVc39933935 = -249887030;    int acvTcvzUVc88305281 = -749866838;    int acvTcvzUVc83568673 = -246472812;    int acvTcvzUVc8672275 = -884914351;    int acvTcvzUVc78879516 = -223100353;    int acvTcvzUVc3267323 = -403239602;    int acvTcvzUVc48327659 = -598328828;    int acvTcvzUVc87224620 = -907061232;    int acvTcvzUVc23173757 = -60050376;    int acvTcvzUVc73690203 = -304372857;    int acvTcvzUVc57273527 = -551450719;    int acvTcvzUVc65153179 = -938072515;    int acvTcvzUVc177804 = -902795525;    int acvTcvzUVc69535284 = 14386532;    int acvTcvzUVc81949026 = -783383835;    int acvTcvzUVc76437783 = -43388436;    int acvTcvzUVc69064346 = -260737383;    int acvTcvzUVc2019346 = -724544209;    int acvTcvzUVc33385724 = 7194730;    int acvTcvzUVc2331723 = -708066885;    int acvTcvzUVc75701618 = -462880022;    int acvTcvzUVc34408834 = -394877767;    int acvTcvzUVc17070395 = -960301856;    int acvTcvzUVc78469411 = 23017392;    int acvTcvzUVc636035 = -178976000;    int acvTcvzUVc58129580 = -66697425;    int acvTcvzUVc6655495 = -521265626;    int acvTcvzUVc92130320 = -86494208;    int acvTcvzUVc5451265 = -152359183;    int acvTcvzUVc69068126 = -375248244;    int acvTcvzUVc57912042 = -451793570;    int acvTcvzUVc36904746 = -458881572;    int acvTcvzUVc48528516 = -670902038;    int acvTcvzUVc65845159 = -621332124;    int acvTcvzUVc41029402 = -476100863;    int acvTcvzUVc89272684 = -138500031;    int acvTcvzUVc98345678 = -547408644;    int acvTcvzUVc16789640 = -136177542;    int acvTcvzUVc69861650 = -52848534;    int acvTcvzUVc76469648 = -908793185;    int acvTcvzUVc866724 = -420044524;    int acvTcvzUVc3819013 = -306002163;    int acvTcvzUVc69660940 = -162200562;    int acvTcvzUVc51465279 = -257666200;    int acvTcvzUVc82206919 = 41998028;    int acvTcvzUVc84816751 = -863511718;    int acvTcvzUVc57072023 = -71493897;    int acvTcvzUVc97008353 = -818099137;    int acvTcvzUVc3138675 = -676387311;    int acvTcvzUVc48719346 = 21824451;    int acvTcvzUVc6525652 = -147899382;    int acvTcvzUVc68673256 = 67046442;    int acvTcvzUVc65365055 = -43935385;     acvTcvzUVc67831881 = acvTcvzUVc2294848;     acvTcvzUVc2294848 = acvTcvzUVc92929051;     acvTcvzUVc92929051 = acvTcvzUVc2928384;     acvTcvzUVc2928384 = acvTcvzUVc76846018;     acvTcvzUVc76846018 = acvTcvzUVc50498621;     acvTcvzUVc50498621 = acvTcvzUVc59526992;     acvTcvzUVc59526992 = acvTcvzUVc58352135;     acvTcvzUVc58352135 = acvTcvzUVc94526260;     acvTcvzUVc94526260 = acvTcvzUVc50760512;     acvTcvzUVc50760512 = acvTcvzUVc13122694;     acvTcvzUVc13122694 = acvTcvzUVc51526106;     acvTcvzUVc51526106 = acvTcvzUVc66745251;     acvTcvzUVc66745251 = acvTcvzUVc27243532;     acvTcvzUVc27243532 = acvTcvzUVc42225877;     acvTcvzUVc42225877 = acvTcvzUVc17949858;     acvTcvzUVc17949858 = acvTcvzUVc50605965;     acvTcvzUVc50605965 = acvTcvzUVc15379407;     acvTcvzUVc15379407 = acvTcvzUVc73199132;     acvTcvzUVc73199132 = acvTcvzUVc77218021;     acvTcvzUVc77218021 = acvTcvzUVc30608394;     acvTcvzUVc30608394 = acvTcvzUVc44658124;     acvTcvzUVc44658124 = acvTcvzUVc28604644;     acvTcvzUVc28604644 = acvTcvzUVc35655525;     acvTcvzUVc35655525 = acvTcvzUVc37775204;     acvTcvzUVc37775204 = acvTcvzUVc76668214;     acvTcvzUVc76668214 = acvTcvzUVc80963336;     acvTcvzUVc80963336 = acvTcvzUVc77577965;     acvTcvzUVc77577965 = acvTcvzUVc81914352;     acvTcvzUVc81914352 = acvTcvzUVc25461914;     acvTcvzUVc25461914 = acvTcvzUVc48741167;     acvTcvzUVc48741167 = acvTcvzUVc79736970;     acvTcvzUVc79736970 = acvTcvzUVc49194383;     acvTcvzUVc49194383 = acvTcvzUVc91043632;     acvTcvzUVc91043632 = acvTcvzUVc92834697;     acvTcvzUVc92834697 = acvTcvzUVc25155482;     acvTcvzUVc25155482 = acvTcvzUVc39480446;     acvTcvzUVc39480446 = acvTcvzUVc49969931;     acvTcvzUVc49969931 = acvTcvzUVc57249826;     acvTcvzUVc57249826 = acvTcvzUVc66543637;     acvTcvzUVc66543637 = acvTcvzUVc85087700;     acvTcvzUVc85087700 = acvTcvzUVc25157129;     acvTcvzUVc25157129 = acvTcvzUVc75589998;     acvTcvzUVc75589998 = acvTcvzUVc70692601;     acvTcvzUVc70692601 = acvTcvzUVc98750778;     acvTcvzUVc98750778 = acvTcvzUVc89246688;     acvTcvzUVc89246688 = acvTcvzUVc10823056;     acvTcvzUVc10823056 = acvTcvzUVc39933935;     acvTcvzUVc39933935 = acvTcvzUVc88305281;     acvTcvzUVc88305281 = acvTcvzUVc83568673;     acvTcvzUVc83568673 = acvTcvzUVc8672275;     acvTcvzUVc8672275 = acvTcvzUVc78879516;     acvTcvzUVc78879516 = acvTcvzUVc3267323;     acvTcvzUVc3267323 = acvTcvzUVc48327659;     acvTcvzUVc48327659 = acvTcvzUVc87224620;     acvTcvzUVc87224620 = acvTcvzUVc23173757;     acvTcvzUVc23173757 = acvTcvzUVc73690203;     acvTcvzUVc73690203 = acvTcvzUVc57273527;     acvTcvzUVc57273527 = acvTcvzUVc65153179;     acvTcvzUVc65153179 = acvTcvzUVc177804;     acvTcvzUVc177804 = acvTcvzUVc69535284;     acvTcvzUVc69535284 = acvTcvzUVc81949026;     acvTcvzUVc81949026 = acvTcvzUVc76437783;     acvTcvzUVc76437783 = acvTcvzUVc69064346;     acvTcvzUVc69064346 = acvTcvzUVc2019346;     acvTcvzUVc2019346 = acvTcvzUVc33385724;     acvTcvzUVc33385724 = acvTcvzUVc2331723;     acvTcvzUVc2331723 = acvTcvzUVc75701618;     acvTcvzUVc75701618 = acvTcvzUVc34408834;     acvTcvzUVc34408834 = acvTcvzUVc17070395;     acvTcvzUVc17070395 = acvTcvzUVc78469411;     acvTcvzUVc78469411 = acvTcvzUVc636035;     acvTcvzUVc636035 = acvTcvzUVc58129580;     acvTcvzUVc58129580 = acvTcvzUVc6655495;     acvTcvzUVc6655495 = acvTcvzUVc92130320;     acvTcvzUVc92130320 = acvTcvzUVc5451265;     acvTcvzUVc5451265 = acvTcvzUVc69068126;     acvTcvzUVc69068126 = acvTcvzUVc57912042;     acvTcvzUVc57912042 = acvTcvzUVc36904746;     acvTcvzUVc36904746 = acvTcvzUVc48528516;     acvTcvzUVc48528516 = acvTcvzUVc65845159;     acvTcvzUVc65845159 = acvTcvzUVc41029402;     acvTcvzUVc41029402 = acvTcvzUVc89272684;     acvTcvzUVc89272684 = acvTcvzUVc98345678;     acvTcvzUVc98345678 = acvTcvzUVc16789640;     acvTcvzUVc16789640 = acvTcvzUVc69861650;     acvTcvzUVc69861650 = acvTcvzUVc76469648;     acvTcvzUVc76469648 = acvTcvzUVc866724;     acvTcvzUVc866724 = acvTcvzUVc3819013;     acvTcvzUVc3819013 = acvTcvzUVc69660940;     acvTcvzUVc69660940 = acvTcvzUVc51465279;     acvTcvzUVc51465279 = acvTcvzUVc82206919;     acvTcvzUVc82206919 = acvTcvzUVc84816751;     acvTcvzUVc84816751 = acvTcvzUVc57072023;     acvTcvzUVc57072023 = acvTcvzUVc97008353;     acvTcvzUVc97008353 = acvTcvzUVc3138675;     acvTcvzUVc3138675 = acvTcvzUVc48719346;     acvTcvzUVc48719346 = acvTcvzUVc6525652;     acvTcvzUVc6525652 = acvTcvzUVc68673256;     acvTcvzUVc68673256 = acvTcvzUVc65365055;     acvTcvzUVc65365055 = acvTcvzUVc67831881;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void giOyxUTpeq54477989() {     int UdXKcFmJqy90218998 = -778339348;    int UdXKcFmJqy86642347 = -562207248;    int UdXKcFmJqy21698330 = -78877735;    int UdXKcFmJqy17935073 = -222823777;    int UdXKcFmJqy75020272 = -434965118;    int UdXKcFmJqy65381737 = -772209737;    int UdXKcFmJqy51682731 = -593738010;    int UdXKcFmJqy33985994 = -407866535;    int UdXKcFmJqy91689391 = -266025315;    int UdXKcFmJqy42524097 = -943234428;    int UdXKcFmJqy50664553 = -401462456;    int UdXKcFmJqy7429055 = 22814470;    int UdXKcFmJqy33753432 = -294660735;    int UdXKcFmJqy69858284 = -244837461;    int UdXKcFmJqy78961154 = -249818625;    int UdXKcFmJqy62205770 = 31897159;    int UdXKcFmJqy429318 = -257840052;    int UdXKcFmJqy21355823 = -592729537;    int UdXKcFmJqy14480763 = -560080356;    int UdXKcFmJqy55734304 = -898899807;    int UdXKcFmJqy31491501 = -365410475;    int UdXKcFmJqy8473342 = -388651322;    int UdXKcFmJqy31897968 = -385371929;    int UdXKcFmJqy62155312 = -851766029;    int UdXKcFmJqy73315451 = -999902856;    int UdXKcFmJqy64971209 = -385006816;    int UdXKcFmJqy28052757 = -583574385;    int UdXKcFmJqy83489415 = -776821322;    int UdXKcFmJqy88806912 = -138138574;    int UdXKcFmJqy34619094 = 58612240;    int UdXKcFmJqy93031971 = -457265038;    int UdXKcFmJqy37278161 = -964104140;    int UdXKcFmJqy30233053 = -547716847;    int UdXKcFmJqy50432320 = -537190245;    int UdXKcFmJqy69064062 = -386007360;    int UdXKcFmJqy66848351 = -977434433;    int UdXKcFmJqy32920159 = -342400642;    int UdXKcFmJqy3596033 = -669209704;    int UdXKcFmJqy5557958 = -375335454;    int UdXKcFmJqy86729352 = 60063366;    int UdXKcFmJqy13845072 = -962948812;    int UdXKcFmJqy13665520 = -979472676;    int UdXKcFmJqy81140190 = -396167349;    int UdXKcFmJqy70136721 = -754861416;    int UdXKcFmJqy66734045 = -638035078;    int UdXKcFmJqy97346969 = -789670789;    int UdXKcFmJqy23120453 = -898190721;    int UdXKcFmJqy84532609 = -6422020;    int UdXKcFmJqy82765186 = -699583536;    int UdXKcFmJqy80652419 = -388493071;    int UdXKcFmJqy17541491 = -139990628;    int UdXKcFmJqy52730774 = -653919635;    int UdXKcFmJqy81581725 = -75150832;    int UdXKcFmJqy8614205 = -874009637;    int UdXKcFmJqy22986488 = 52234653;    int UdXKcFmJqy81745656 = -289688026;    int UdXKcFmJqy54744380 = -76835320;    int UdXKcFmJqy59543018 = -227111706;    int UdXKcFmJqy44619621 = -222920922;    int UdXKcFmJqy10049064 = 50041697;    int UdXKcFmJqy37328980 = -88635353;    int UdXKcFmJqy68193315 = -816916688;    int UdXKcFmJqy45179081 = -169727961;    int UdXKcFmJqy57070298 = -224637556;    int UdXKcFmJqy49492125 = -385969390;    int UdXKcFmJqy13386393 = -437358316;    int UdXKcFmJqy77196001 = -429468683;    int UdXKcFmJqy83321111 = -757470491;    int UdXKcFmJqy794222 = -858830102;    int UdXKcFmJqy12112803 = -272384192;    int UdXKcFmJqy29285612 = -625702199;    int UdXKcFmJqy96833285 = -588630348;    int UdXKcFmJqy15797866 = -117394084;    int UdXKcFmJqy27751411 = -520143722;    int UdXKcFmJqy41889233 = -935950995;    int UdXKcFmJqy17825981 = -385937800;    int UdXKcFmJqy27333152 = -992483974;    int UdXKcFmJqy61761246 = -630510513;    int UdXKcFmJqy95421266 = -113730952;    int UdXKcFmJqy75968481 = -110232067;    int UdXKcFmJqy41850756 = -486816095;    int UdXKcFmJqy43520148 = -477152365;    int UdXKcFmJqy724230 = 22762213;    int UdXKcFmJqy8154494 = -749645504;    int UdXKcFmJqy17077603 = -801397132;    int UdXKcFmJqy40301198 = -803345404;    int UdXKcFmJqy55696435 = -788953309;    int UdXKcFmJqy21618849 = -673707211;    int UdXKcFmJqy27445833 = -489424898;    int UdXKcFmJqy87318406 = 3680665;    int UdXKcFmJqy12103972 = -800599114;    int UdXKcFmJqy73377140 = -15288936;    int UdXKcFmJqy58976411 = -346288783;    int UdXKcFmJqy95508894 = -325377152;    int UdXKcFmJqy49400372 = -851301282;    int UdXKcFmJqy45651756 = -46032124;    int UdXKcFmJqy68486439 = -709744715;    int UdXKcFmJqy24069892 = -71529794;    int UdXKcFmJqy20644596 = -268892026;    int UdXKcFmJqy53347653 = -778339348;     UdXKcFmJqy90218998 = UdXKcFmJqy86642347;     UdXKcFmJqy86642347 = UdXKcFmJqy21698330;     UdXKcFmJqy21698330 = UdXKcFmJqy17935073;     UdXKcFmJqy17935073 = UdXKcFmJqy75020272;     UdXKcFmJqy75020272 = UdXKcFmJqy65381737;     UdXKcFmJqy65381737 = UdXKcFmJqy51682731;     UdXKcFmJqy51682731 = UdXKcFmJqy33985994;     UdXKcFmJqy33985994 = UdXKcFmJqy91689391;     UdXKcFmJqy91689391 = UdXKcFmJqy42524097;     UdXKcFmJqy42524097 = UdXKcFmJqy50664553;     UdXKcFmJqy50664553 = UdXKcFmJqy7429055;     UdXKcFmJqy7429055 = UdXKcFmJqy33753432;     UdXKcFmJqy33753432 = UdXKcFmJqy69858284;     UdXKcFmJqy69858284 = UdXKcFmJqy78961154;     UdXKcFmJqy78961154 = UdXKcFmJqy62205770;     UdXKcFmJqy62205770 = UdXKcFmJqy429318;     UdXKcFmJqy429318 = UdXKcFmJqy21355823;     UdXKcFmJqy21355823 = UdXKcFmJqy14480763;     UdXKcFmJqy14480763 = UdXKcFmJqy55734304;     UdXKcFmJqy55734304 = UdXKcFmJqy31491501;     UdXKcFmJqy31491501 = UdXKcFmJqy8473342;     UdXKcFmJqy8473342 = UdXKcFmJqy31897968;     UdXKcFmJqy31897968 = UdXKcFmJqy62155312;     UdXKcFmJqy62155312 = UdXKcFmJqy73315451;     UdXKcFmJqy73315451 = UdXKcFmJqy64971209;     UdXKcFmJqy64971209 = UdXKcFmJqy28052757;     UdXKcFmJqy28052757 = UdXKcFmJqy83489415;     UdXKcFmJqy83489415 = UdXKcFmJqy88806912;     UdXKcFmJqy88806912 = UdXKcFmJqy34619094;     UdXKcFmJqy34619094 = UdXKcFmJqy93031971;     UdXKcFmJqy93031971 = UdXKcFmJqy37278161;     UdXKcFmJqy37278161 = UdXKcFmJqy30233053;     UdXKcFmJqy30233053 = UdXKcFmJqy50432320;     UdXKcFmJqy50432320 = UdXKcFmJqy69064062;     UdXKcFmJqy69064062 = UdXKcFmJqy66848351;     UdXKcFmJqy66848351 = UdXKcFmJqy32920159;     UdXKcFmJqy32920159 = UdXKcFmJqy3596033;     UdXKcFmJqy3596033 = UdXKcFmJqy5557958;     UdXKcFmJqy5557958 = UdXKcFmJqy86729352;     UdXKcFmJqy86729352 = UdXKcFmJqy13845072;     UdXKcFmJqy13845072 = UdXKcFmJqy13665520;     UdXKcFmJqy13665520 = UdXKcFmJqy81140190;     UdXKcFmJqy81140190 = UdXKcFmJqy70136721;     UdXKcFmJqy70136721 = UdXKcFmJqy66734045;     UdXKcFmJqy66734045 = UdXKcFmJqy97346969;     UdXKcFmJqy97346969 = UdXKcFmJqy23120453;     UdXKcFmJqy23120453 = UdXKcFmJqy84532609;     UdXKcFmJqy84532609 = UdXKcFmJqy82765186;     UdXKcFmJqy82765186 = UdXKcFmJqy80652419;     UdXKcFmJqy80652419 = UdXKcFmJqy17541491;     UdXKcFmJqy17541491 = UdXKcFmJqy52730774;     UdXKcFmJqy52730774 = UdXKcFmJqy81581725;     UdXKcFmJqy81581725 = UdXKcFmJqy8614205;     UdXKcFmJqy8614205 = UdXKcFmJqy22986488;     UdXKcFmJqy22986488 = UdXKcFmJqy81745656;     UdXKcFmJqy81745656 = UdXKcFmJqy54744380;     UdXKcFmJqy54744380 = UdXKcFmJqy59543018;     UdXKcFmJqy59543018 = UdXKcFmJqy44619621;     UdXKcFmJqy44619621 = UdXKcFmJqy10049064;     UdXKcFmJqy10049064 = UdXKcFmJqy37328980;     UdXKcFmJqy37328980 = UdXKcFmJqy68193315;     UdXKcFmJqy68193315 = UdXKcFmJqy45179081;     UdXKcFmJqy45179081 = UdXKcFmJqy57070298;     UdXKcFmJqy57070298 = UdXKcFmJqy49492125;     UdXKcFmJqy49492125 = UdXKcFmJqy13386393;     UdXKcFmJqy13386393 = UdXKcFmJqy77196001;     UdXKcFmJqy77196001 = UdXKcFmJqy83321111;     UdXKcFmJqy83321111 = UdXKcFmJqy794222;     UdXKcFmJqy794222 = UdXKcFmJqy12112803;     UdXKcFmJqy12112803 = UdXKcFmJqy29285612;     UdXKcFmJqy29285612 = UdXKcFmJqy96833285;     UdXKcFmJqy96833285 = UdXKcFmJqy15797866;     UdXKcFmJqy15797866 = UdXKcFmJqy27751411;     UdXKcFmJqy27751411 = UdXKcFmJqy41889233;     UdXKcFmJqy41889233 = UdXKcFmJqy17825981;     UdXKcFmJqy17825981 = UdXKcFmJqy27333152;     UdXKcFmJqy27333152 = UdXKcFmJqy61761246;     UdXKcFmJqy61761246 = UdXKcFmJqy95421266;     UdXKcFmJqy95421266 = UdXKcFmJqy75968481;     UdXKcFmJqy75968481 = UdXKcFmJqy41850756;     UdXKcFmJqy41850756 = UdXKcFmJqy43520148;     UdXKcFmJqy43520148 = UdXKcFmJqy724230;     UdXKcFmJqy724230 = UdXKcFmJqy8154494;     UdXKcFmJqy8154494 = UdXKcFmJqy17077603;     UdXKcFmJqy17077603 = UdXKcFmJqy40301198;     UdXKcFmJqy40301198 = UdXKcFmJqy55696435;     UdXKcFmJqy55696435 = UdXKcFmJqy21618849;     UdXKcFmJqy21618849 = UdXKcFmJqy27445833;     UdXKcFmJqy27445833 = UdXKcFmJqy87318406;     UdXKcFmJqy87318406 = UdXKcFmJqy12103972;     UdXKcFmJqy12103972 = UdXKcFmJqy73377140;     UdXKcFmJqy73377140 = UdXKcFmJqy58976411;     UdXKcFmJqy58976411 = UdXKcFmJqy95508894;     UdXKcFmJqy95508894 = UdXKcFmJqy49400372;     UdXKcFmJqy49400372 = UdXKcFmJqy45651756;     UdXKcFmJqy45651756 = UdXKcFmJqy68486439;     UdXKcFmJqy68486439 = UdXKcFmJqy24069892;     UdXKcFmJqy24069892 = UdXKcFmJqy20644596;     UdXKcFmJqy20644596 = UdXKcFmJqy53347653;     UdXKcFmJqy53347653 = UdXKcFmJqy90218998;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void FLOnFaVZSj91122963() {     int QBQABgHbxT12606116 = -412743311;    int QBQABgHbxT70989848 = -510750302;    int QBQABgHbxT50467609 = -318531579;    int QBQABgHbxT32941761 = -229231308;    int QBQABgHbxT73194527 = 73977881;    int QBQABgHbxT80264854 = -632818115;    int QBQABgHbxT43838470 = -415725316;    int QBQABgHbxT9619853 = -878463179;    int QBQABgHbxT88852522 = -150221354;    int QBQABgHbxT34287681 = -685975761;    int QBQABgHbxT88206413 = -398086857;    int QBQABgHbxT63332003 = -227930823;    int QBQABgHbxT761613 = -913378055;    int QBQABgHbxT12473037 = -772546218;    int QBQABgHbxT15696431 = -977296338;    int QBQABgHbxT6461683 = -349770383;    int QBQABgHbxT50252669 = -535119872;    int QBQABgHbxT27332240 = 55527772;    int QBQABgHbxT55762394 = -695182481;    int QBQABgHbxT34250588 = -51534260;    int QBQABgHbxT32374608 = -356897784;    int QBQABgHbxT72288560 = -893417635;    int QBQABgHbxT35191291 = -561452521;    int QBQABgHbxT88655099 = -315758886;    int QBQABgHbxT8855698 = -621461980;    int QBQABgHbxT53274204 = -828901040;    int QBQABgHbxT75142177 = -341160877;    int QBQABgHbxT89400865 = -565275776;    int QBQABgHbxT95699473 = -482395694;    int QBQABgHbxT43776273 = -961683626;    int QBQABgHbxT37322777 = -538581190;    int QBQABgHbxT94819352 = -516175495;    int QBQABgHbxT11271724 = 22939658;    int QBQABgHbxT9821008 = -861317095;    int QBQABgHbxT45293427 = -449763782;    int QBQABgHbxT8541221 = -192829809;    int QBQABgHbxT26359871 = -75348593;    int QBQABgHbxT57222133 = -536835176;    int QBQABgHbxT53866089 = -776381485;    int QBQABgHbxT6915067 = -76160663;    int QBQABgHbxT42602443 = -366126478;    int QBQABgHbxT2173912 = -737381367;    int QBQABgHbxT86690381 = -283697933;    int QBQABgHbxT69580840 = -752225065;    int QBQABgHbxT34717312 = -447178555;    int QBQABgHbxT5447252 = -971899884;    int QBQABgHbxT35417850 = -376600974;    int QBQABgHbxT29131284 = -862957011;    int QBQABgHbxT77225091 = -649300234;    int QBQABgHbxT77736165 = -530513330;    int QBQABgHbxT26410708 = -495066905;    int QBQABgHbxT26582031 = 15261083;    int QBQABgHbxT59896129 = -847062062;    int QBQABgHbxT68900749 = -49690446;    int QBQABgHbxT58748355 = -88469462;    int QBQABgHbxT40317555 = -519325676;    int QBQABgHbxT35798558 = -949297782;    int QBQABgHbxT61812510 = 97227307;    int QBQABgHbxT24086063 = -607769329;    int QBQABgHbxT19920323 = -97121080;    int QBQABgHbxT5122677 = -191657238;    int QBQABgHbxT54437604 = -850449541;    int QBQABgHbxT13920379 = -296067486;    int QBQABgHbxT45076249 = -188537728;    int QBQABgHbxT96964904 = -47394571;    int QBQABgHbxT93387061 = -881911362;    int QBQABgHbxT52060280 = -150870482;    int QBQABgHbxT90940605 = 47939039;    int QBQABgHbxT67179609 = -222782436;    int QBQABgHbxT7155211 = -684466529;    int QBQABgHbxT80101812 = -174421790;    int QBQABgHbxT93030535 = -998284696;    int QBQABgHbxT73466150 = -168090743;    int QBQABgHbxT48847327 = -519021818;    int QBQABgHbxT91648145 = -685407783;    int QBQABgHbxT30200697 = -619516417;    int QBQABgHbxT85598178 = -509719703;    int QBQABgHbxT65610450 = -809227456;    int QBQABgHbxT53937787 = -868580331;    int QBQABgHbxT3408447 = -649562096;    int QBQABgHbxT17856354 = -352300066;    int QBQABgHbxT46010894 = -478203867;    int QBQABgHbxT12175775 = -915975542;    int QBQABgHbxT17963309 = -951882365;    int QBQABgHbxT17365566 = -366616721;    int QBQABgHbxT10740746 = -453842274;    int QBQABgHbxT34923223 = -669113434;    int QBQABgHbxT42370974 = -927369897;    int QBQABgHbxT51072652 = -672847633;    int QBQABgHbxT4975872 = -930438107;    int QBQABgHbxT72742663 = -243532027;    int QBQABgHbxT64547361 = -72575901;    int QBQABgHbxT33136070 = -929065848;    int QBQABgHbxT33945766 = -579260406;    int QBQABgHbxT1792390 = -884503426;    int QBQABgHbxT88164838 = -515676938;    int QBQABgHbxT88253532 = -341313881;    int QBQABgHbxT41614133 = 4839795;    int QBQABgHbxT72615936 = -604830494;    int QBQABgHbxT41330251 = -412743311;     QBQABgHbxT12606116 = QBQABgHbxT70989848;     QBQABgHbxT70989848 = QBQABgHbxT50467609;     QBQABgHbxT50467609 = QBQABgHbxT32941761;     QBQABgHbxT32941761 = QBQABgHbxT73194527;     QBQABgHbxT73194527 = QBQABgHbxT80264854;     QBQABgHbxT80264854 = QBQABgHbxT43838470;     QBQABgHbxT43838470 = QBQABgHbxT9619853;     QBQABgHbxT9619853 = QBQABgHbxT88852522;     QBQABgHbxT88852522 = QBQABgHbxT34287681;     QBQABgHbxT34287681 = QBQABgHbxT88206413;     QBQABgHbxT88206413 = QBQABgHbxT63332003;     QBQABgHbxT63332003 = QBQABgHbxT761613;     QBQABgHbxT761613 = QBQABgHbxT12473037;     QBQABgHbxT12473037 = QBQABgHbxT15696431;     QBQABgHbxT15696431 = QBQABgHbxT6461683;     QBQABgHbxT6461683 = QBQABgHbxT50252669;     QBQABgHbxT50252669 = QBQABgHbxT27332240;     QBQABgHbxT27332240 = QBQABgHbxT55762394;     QBQABgHbxT55762394 = QBQABgHbxT34250588;     QBQABgHbxT34250588 = QBQABgHbxT32374608;     QBQABgHbxT32374608 = QBQABgHbxT72288560;     QBQABgHbxT72288560 = QBQABgHbxT35191291;     QBQABgHbxT35191291 = QBQABgHbxT88655099;     QBQABgHbxT88655099 = QBQABgHbxT8855698;     QBQABgHbxT8855698 = QBQABgHbxT53274204;     QBQABgHbxT53274204 = QBQABgHbxT75142177;     QBQABgHbxT75142177 = QBQABgHbxT89400865;     QBQABgHbxT89400865 = QBQABgHbxT95699473;     QBQABgHbxT95699473 = QBQABgHbxT43776273;     QBQABgHbxT43776273 = QBQABgHbxT37322777;     QBQABgHbxT37322777 = QBQABgHbxT94819352;     QBQABgHbxT94819352 = QBQABgHbxT11271724;     QBQABgHbxT11271724 = QBQABgHbxT9821008;     QBQABgHbxT9821008 = QBQABgHbxT45293427;     QBQABgHbxT45293427 = QBQABgHbxT8541221;     QBQABgHbxT8541221 = QBQABgHbxT26359871;     QBQABgHbxT26359871 = QBQABgHbxT57222133;     QBQABgHbxT57222133 = QBQABgHbxT53866089;     QBQABgHbxT53866089 = QBQABgHbxT6915067;     QBQABgHbxT6915067 = QBQABgHbxT42602443;     QBQABgHbxT42602443 = QBQABgHbxT2173912;     QBQABgHbxT2173912 = QBQABgHbxT86690381;     QBQABgHbxT86690381 = QBQABgHbxT69580840;     QBQABgHbxT69580840 = QBQABgHbxT34717312;     QBQABgHbxT34717312 = QBQABgHbxT5447252;     QBQABgHbxT5447252 = QBQABgHbxT35417850;     QBQABgHbxT35417850 = QBQABgHbxT29131284;     QBQABgHbxT29131284 = QBQABgHbxT77225091;     QBQABgHbxT77225091 = QBQABgHbxT77736165;     QBQABgHbxT77736165 = QBQABgHbxT26410708;     QBQABgHbxT26410708 = QBQABgHbxT26582031;     QBQABgHbxT26582031 = QBQABgHbxT59896129;     QBQABgHbxT59896129 = QBQABgHbxT68900749;     QBQABgHbxT68900749 = QBQABgHbxT58748355;     QBQABgHbxT58748355 = QBQABgHbxT40317555;     QBQABgHbxT40317555 = QBQABgHbxT35798558;     QBQABgHbxT35798558 = QBQABgHbxT61812510;     QBQABgHbxT61812510 = QBQABgHbxT24086063;     QBQABgHbxT24086063 = QBQABgHbxT19920323;     QBQABgHbxT19920323 = QBQABgHbxT5122677;     QBQABgHbxT5122677 = QBQABgHbxT54437604;     QBQABgHbxT54437604 = QBQABgHbxT13920379;     QBQABgHbxT13920379 = QBQABgHbxT45076249;     QBQABgHbxT45076249 = QBQABgHbxT96964904;     QBQABgHbxT96964904 = QBQABgHbxT93387061;     QBQABgHbxT93387061 = QBQABgHbxT52060280;     QBQABgHbxT52060280 = QBQABgHbxT90940605;     QBQABgHbxT90940605 = QBQABgHbxT67179609;     QBQABgHbxT67179609 = QBQABgHbxT7155211;     QBQABgHbxT7155211 = QBQABgHbxT80101812;     QBQABgHbxT80101812 = QBQABgHbxT93030535;     QBQABgHbxT93030535 = QBQABgHbxT73466150;     QBQABgHbxT73466150 = QBQABgHbxT48847327;     QBQABgHbxT48847327 = QBQABgHbxT91648145;     QBQABgHbxT91648145 = QBQABgHbxT30200697;     QBQABgHbxT30200697 = QBQABgHbxT85598178;     QBQABgHbxT85598178 = QBQABgHbxT65610450;     QBQABgHbxT65610450 = QBQABgHbxT53937787;     QBQABgHbxT53937787 = QBQABgHbxT3408447;     QBQABgHbxT3408447 = QBQABgHbxT17856354;     QBQABgHbxT17856354 = QBQABgHbxT46010894;     QBQABgHbxT46010894 = QBQABgHbxT12175775;     QBQABgHbxT12175775 = QBQABgHbxT17963309;     QBQABgHbxT17963309 = QBQABgHbxT17365566;     QBQABgHbxT17365566 = QBQABgHbxT10740746;     QBQABgHbxT10740746 = QBQABgHbxT34923223;     QBQABgHbxT34923223 = QBQABgHbxT42370974;     QBQABgHbxT42370974 = QBQABgHbxT51072652;     QBQABgHbxT51072652 = QBQABgHbxT4975872;     QBQABgHbxT4975872 = QBQABgHbxT72742663;     QBQABgHbxT72742663 = QBQABgHbxT64547361;     QBQABgHbxT64547361 = QBQABgHbxT33136070;     QBQABgHbxT33136070 = QBQABgHbxT33945766;     QBQABgHbxT33945766 = QBQABgHbxT1792390;     QBQABgHbxT1792390 = QBQABgHbxT88164838;     QBQABgHbxT88164838 = QBQABgHbxT88253532;     QBQABgHbxT88253532 = QBQABgHbxT41614133;     QBQABgHbxT41614133 = QBQABgHbxT72615936;     QBQABgHbxT72615936 = QBQABgHbxT41330251;     QBQABgHbxT41330251 = QBQABgHbxT12606116;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void WIJMWWHUrT27767938() {     int GeQoZjhMnI34993233 = -47147274;    int GeQoZjhMnI55337348 = -459293357;    int GeQoZjhMnI79236887 = -558185423;    int GeQoZjhMnI47948449 = -235638840;    int GeQoZjhMnI71368781 = -517079121;    int GeQoZjhMnI95147971 = -493426493;    int GeQoZjhMnI35994209 = -237712622;    int GeQoZjhMnI85253711 = -249059823;    int GeQoZjhMnI86015653 = -34417393;    int GeQoZjhMnI26051266 = -428717094;    int GeQoZjhMnI25748273 = -394711258;    int GeQoZjhMnI19234953 = -478676116;    int GeQoZjhMnI67769793 = -432095375;    int GeQoZjhMnI55087788 = -200254975;    int GeQoZjhMnI52431708 = -604774051;    int GeQoZjhMnI50717595 = -731437926;    int GeQoZjhMnI76021 = -812399692;    int GeQoZjhMnI33308656 = -396214919;    int GeQoZjhMnI97044024 = -830284606;    int GeQoZjhMnI12766871 = -304168714;    int GeQoZjhMnI33257716 = -348385092;    int GeQoZjhMnI36103779 = -298183949;    int GeQoZjhMnI38484614 = -737533113;    int GeQoZjhMnI15154886 = -879751743;    int GeQoZjhMnI44395945 = -243021105;    int GeQoZjhMnI41577199 = -172795264;    int GeQoZjhMnI22231598 = -98747370;    int GeQoZjhMnI95312315 = -353730229;    int GeQoZjhMnI2592035 = -826652813;    int GeQoZjhMnI52933452 = -881979493;    int GeQoZjhMnI81613582 = -619897343;    int GeQoZjhMnI52360543 = -68246850;    int GeQoZjhMnI92310394 = -506403837;    int GeQoZjhMnI69209694 = -85443945;    int GeQoZjhMnI21522792 = -513520205;    int GeQoZjhMnI50234090 = -508225186;    int GeQoZjhMnI19799583 = -908296545;    int GeQoZjhMnI10848234 = -404460648;    int GeQoZjhMnI2174221 = -77427517;    int GeQoZjhMnI27100782 = -212384693;    int GeQoZjhMnI71359813 = -869304144;    int GeQoZjhMnI90682302 = -495290058;    int GeQoZjhMnI92240573 = -171228516;    int GeQoZjhMnI69024959 = -749588714;    int GeQoZjhMnI2700579 = -256322033;    int GeQoZjhMnI13547534 = -54128980;    int GeQoZjhMnI47715248 = -955011227;    int GeQoZjhMnI73729958 = -619492002;    int GeQoZjhMnI71684996 = -599016933;    int GeQoZjhMnI74819910 = -672533589;    int GeQoZjhMnI35279924 = -850143182;    int GeQoZjhMnI433288 = -415558199;    int GeQoZjhMnI38210532 = -518973292;    int GeQoZjhMnI29187295 = -325371254;    int GeQoZjhMnI94510222 = -229173577;    int GeQoZjhMnI98889454 = -748963326;    int GeQoZjhMnI16852735 = -721760245;    int GeQoZjhMnI64082001 = -678433680;    int GeQoZjhMnI3552505 = -992617736;    int GeQoZjhMnI29791583 = -244283857;    int GeQoZjhMnI72916373 = -294679123;    int GeQoZjhMnI40681893 = -883982393;    int GeQoZjhMnI82661676 = -422407011;    int GeQoZjhMnI33082201 = -152437901;    int GeQoZjhMnI44437684 = -808819752;    int GeQoZjhMnI73387730 = -226464409;    int GeQoZjhMnI26924559 = -972272280;    int GeQoZjhMnI98560098 = -246651430;    int GeQoZjhMnI33564997 = -686734770;    int GeQoZjhMnI2197619 = 3451134;    int GeQoZjhMnI30918013 = -823141381;    int GeQoZjhMnI89227786 = -307939044;    int GeQoZjhMnI31134436 = -218787402;    int GeQoZjhMnI69943243 = -517899914;    int GeQoZjhMnI41407058 = -434864570;    int GeQoZjhMnI42575413 = -853095035;    int GeQoZjhMnI43863205 = -26955433;    int GeQoZjhMnI69459654 = -987944399;    int GeQoZjhMnI12454308 = -523429711;    int GeQoZjhMnI30848411 = -88892126;    int GeQoZjhMnI93861950 = -217784037;    int GeQoZjhMnI48501639 = -479255369;    int GeQoZjhMnI23627320 = -754713297;    int GeQoZjhMnI27772124 = -54119225;    int GeQoZjhMnI17653529 = 68163689;    int GeQoZjhMnI81180294 = -104339144;    int GeQoZjhMnI14150011 = -549273558;    int GeQoZjhMnI63123099 = -81032583;    int GeQoZjhMnI74699472 = -856270368;    int GeQoZjhMnI22633338 = -764556880;    int GeQoZjhMnI33381355 = -786464941;    int GeQoZjhMnI55717582 = -129862865;    int GeQoZjhMnI7295730 = -411842913;    int GeQoZjhMnI72382638 = -833143660;    int GeQoZjhMnI54184408 = -917705570;    int GeQoZjhMnI30677921 = -985321751;    int GeQoZjhMnI8020626 = 27116953;    int GeQoZjhMnI59158373 = 81209384;    int GeQoZjhMnI24587276 = -940768962;    int GeQoZjhMnI29312849 = -47147274;     GeQoZjhMnI34993233 = GeQoZjhMnI55337348;     GeQoZjhMnI55337348 = GeQoZjhMnI79236887;     GeQoZjhMnI79236887 = GeQoZjhMnI47948449;     GeQoZjhMnI47948449 = GeQoZjhMnI71368781;     GeQoZjhMnI71368781 = GeQoZjhMnI95147971;     GeQoZjhMnI95147971 = GeQoZjhMnI35994209;     GeQoZjhMnI35994209 = GeQoZjhMnI85253711;     GeQoZjhMnI85253711 = GeQoZjhMnI86015653;     GeQoZjhMnI86015653 = GeQoZjhMnI26051266;     GeQoZjhMnI26051266 = GeQoZjhMnI25748273;     GeQoZjhMnI25748273 = GeQoZjhMnI19234953;     GeQoZjhMnI19234953 = GeQoZjhMnI67769793;     GeQoZjhMnI67769793 = GeQoZjhMnI55087788;     GeQoZjhMnI55087788 = GeQoZjhMnI52431708;     GeQoZjhMnI52431708 = GeQoZjhMnI50717595;     GeQoZjhMnI50717595 = GeQoZjhMnI76021;     GeQoZjhMnI76021 = GeQoZjhMnI33308656;     GeQoZjhMnI33308656 = GeQoZjhMnI97044024;     GeQoZjhMnI97044024 = GeQoZjhMnI12766871;     GeQoZjhMnI12766871 = GeQoZjhMnI33257716;     GeQoZjhMnI33257716 = GeQoZjhMnI36103779;     GeQoZjhMnI36103779 = GeQoZjhMnI38484614;     GeQoZjhMnI38484614 = GeQoZjhMnI15154886;     GeQoZjhMnI15154886 = GeQoZjhMnI44395945;     GeQoZjhMnI44395945 = GeQoZjhMnI41577199;     GeQoZjhMnI41577199 = GeQoZjhMnI22231598;     GeQoZjhMnI22231598 = GeQoZjhMnI95312315;     GeQoZjhMnI95312315 = GeQoZjhMnI2592035;     GeQoZjhMnI2592035 = GeQoZjhMnI52933452;     GeQoZjhMnI52933452 = GeQoZjhMnI81613582;     GeQoZjhMnI81613582 = GeQoZjhMnI52360543;     GeQoZjhMnI52360543 = GeQoZjhMnI92310394;     GeQoZjhMnI92310394 = GeQoZjhMnI69209694;     GeQoZjhMnI69209694 = GeQoZjhMnI21522792;     GeQoZjhMnI21522792 = GeQoZjhMnI50234090;     GeQoZjhMnI50234090 = GeQoZjhMnI19799583;     GeQoZjhMnI19799583 = GeQoZjhMnI10848234;     GeQoZjhMnI10848234 = GeQoZjhMnI2174221;     GeQoZjhMnI2174221 = GeQoZjhMnI27100782;     GeQoZjhMnI27100782 = GeQoZjhMnI71359813;     GeQoZjhMnI71359813 = GeQoZjhMnI90682302;     GeQoZjhMnI90682302 = GeQoZjhMnI92240573;     GeQoZjhMnI92240573 = GeQoZjhMnI69024959;     GeQoZjhMnI69024959 = GeQoZjhMnI2700579;     GeQoZjhMnI2700579 = GeQoZjhMnI13547534;     GeQoZjhMnI13547534 = GeQoZjhMnI47715248;     GeQoZjhMnI47715248 = GeQoZjhMnI73729958;     GeQoZjhMnI73729958 = GeQoZjhMnI71684996;     GeQoZjhMnI71684996 = GeQoZjhMnI74819910;     GeQoZjhMnI74819910 = GeQoZjhMnI35279924;     GeQoZjhMnI35279924 = GeQoZjhMnI433288;     GeQoZjhMnI433288 = GeQoZjhMnI38210532;     GeQoZjhMnI38210532 = GeQoZjhMnI29187295;     GeQoZjhMnI29187295 = GeQoZjhMnI94510222;     GeQoZjhMnI94510222 = GeQoZjhMnI98889454;     GeQoZjhMnI98889454 = GeQoZjhMnI16852735;     GeQoZjhMnI16852735 = GeQoZjhMnI64082001;     GeQoZjhMnI64082001 = GeQoZjhMnI3552505;     GeQoZjhMnI3552505 = GeQoZjhMnI29791583;     GeQoZjhMnI29791583 = GeQoZjhMnI72916373;     GeQoZjhMnI72916373 = GeQoZjhMnI40681893;     GeQoZjhMnI40681893 = GeQoZjhMnI82661676;     GeQoZjhMnI82661676 = GeQoZjhMnI33082201;     GeQoZjhMnI33082201 = GeQoZjhMnI44437684;     GeQoZjhMnI44437684 = GeQoZjhMnI73387730;     GeQoZjhMnI73387730 = GeQoZjhMnI26924559;     GeQoZjhMnI26924559 = GeQoZjhMnI98560098;     GeQoZjhMnI98560098 = GeQoZjhMnI33564997;     GeQoZjhMnI33564997 = GeQoZjhMnI2197619;     GeQoZjhMnI2197619 = GeQoZjhMnI30918013;     GeQoZjhMnI30918013 = GeQoZjhMnI89227786;     GeQoZjhMnI89227786 = GeQoZjhMnI31134436;     GeQoZjhMnI31134436 = GeQoZjhMnI69943243;     GeQoZjhMnI69943243 = GeQoZjhMnI41407058;     GeQoZjhMnI41407058 = GeQoZjhMnI42575413;     GeQoZjhMnI42575413 = GeQoZjhMnI43863205;     GeQoZjhMnI43863205 = GeQoZjhMnI69459654;     GeQoZjhMnI69459654 = GeQoZjhMnI12454308;     GeQoZjhMnI12454308 = GeQoZjhMnI30848411;     GeQoZjhMnI30848411 = GeQoZjhMnI93861950;     GeQoZjhMnI93861950 = GeQoZjhMnI48501639;     GeQoZjhMnI48501639 = GeQoZjhMnI23627320;     GeQoZjhMnI23627320 = GeQoZjhMnI27772124;     GeQoZjhMnI27772124 = GeQoZjhMnI17653529;     GeQoZjhMnI17653529 = GeQoZjhMnI81180294;     GeQoZjhMnI81180294 = GeQoZjhMnI14150011;     GeQoZjhMnI14150011 = GeQoZjhMnI63123099;     GeQoZjhMnI63123099 = GeQoZjhMnI74699472;     GeQoZjhMnI74699472 = GeQoZjhMnI22633338;     GeQoZjhMnI22633338 = GeQoZjhMnI33381355;     GeQoZjhMnI33381355 = GeQoZjhMnI55717582;     GeQoZjhMnI55717582 = GeQoZjhMnI7295730;     GeQoZjhMnI7295730 = GeQoZjhMnI72382638;     GeQoZjhMnI72382638 = GeQoZjhMnI54184408;     GeQoZjhMnI54184408 = GeQoZjhMnI30677921;     GeQoZjhMnI30677921 = GeQoZjhMnI8020626;     GeQoZjhMnI8020626 = GeQoZjhMnI59158373;     GeQoZjhMnI59158373 = GeQoZjhMnI24587276;     GeQoZjhMnI24587276 = GeQoZjhMnI29312849;     GeQoZjhMnI29312849 = GeQoZjhMnI34993233;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void RoCaHTioCI64412912() {     int VxVimadtSX57380350 = -781551237;    int VxVimadtSX39684849 = -407836412;    int VxVimadtSX8006166 = -797839267;    int VxVimadtSX62955137 = -242046372;    int VxVimadtSX69543035 = -8136122;    int VxVimadtSX10031089 = -354034871;    int VxVimadtSX28149948 = -59699928;    int VxVimadtSX60887570 = -719656468;    int VxVimadtSX83178784 = 81386568;    int VxVimadtSX17814851 = -171458427;    int VxVimadtSX63290132 = -391335659;    int VxVimadtSX75137901 = -729421410;    int VxVimadtSX34777974 = 49187306;    int VxVimadtSX97702540 = -727963732;    int VxVimadtSX89166985 = -232251764;    int VxVimadtSX94973507 = -13105468;    int VxVimadtSX49899372 = 10320488;    int VxVimadtSX39285072 = -847957610;    int VxVimadtSX38325656 = -965386731;    int VxVimadtSX91283154 = -556803168;    int VxVimadtSX34140823 = -339872401;    int VxVimadtSX99918997 = -802950262;    int VxVimadtSX41777937 = -913613705;    int VxVimadtSX41654673 = -343744601;    int VxVimadtSX79936191 = -964580229;    int VxVimadtSX29880193 = -616689488;    int VxVimadtSX69321018 = -956333863;    int VxVimadtSX1223766 = -142184683;    int VxVimadtSX9484596 = -70909932;    int VxVimadtSX62090632 = -802275359;    int VxVimadtSX25904387 = -701213495;    int VxVimadtSX9901734 = -720318204;    int VxVimadtSX73349064 = 64252668;    int VxVimadtSX28598382 = -409570795;    int VxVimadtSX97752156 = -577276627;    int VxVimadtSX91926958 = -823620562;    int VxVimadtSX13239296 = -641244496;    int VxVimadtSX64474335 = -272086120;    int VxVimadtSX50482352 = -478473549;    int VxVimadtSX47286496 = -348608722;    int VxVimadtSX117185 = -272481810;    int VxVimadtSX79190693 = -253198749;    int VxVimadtSX97790765 = -58759100;    int VxVimadtSX68469079 = -746952363;    int VxVimadtSX70683845 = -65465511;    int VxVimadtSX21647816 = -236358075;    int VxVimadtSX60012645 = -433421480;    int VxVimadtSX18328633 = -376026993;    int VxVimadtSX66144901 = -548733631;    int VxVimadtSX71903656 = -814553847;    int VxVimadtSX44149140 = -105219459;    int VxVimadtSX74284545 = -846377481;    int VxVimadtSX16524936 = -190884522;    int VxVimadtSX89473840 = -601052063;    int VxVimadtSX30272090 = -369877693;    int VxVimadtSX57461353 = -978600975;    int VxVimadtSX97906911 = -494222708;    int VxVimadtSX66351493 = -354094667;    int VxVimadtSX83018946 = -277466143;    int VxVimadtSX39662842 = -391446635;    int VxVimadtSX40710070 = -397701008;    int VxVimadtSX26926182 = -917515246;    int VxVimadtSX51402975 = -548746536;    int VxVimadtSX21088153 = -116338073;    int VxVimadtSX91910463 = -470244933;    int VxVimadtSX53388399 = -671017455;    int VxVimadtSX1788838 = -693674079;    int VxVimadtSX6179592 = -541241900;    int VxVimadtSX99950384 = -50687105;    int VxVimadtSX97240026 = -408631202;    int VxVimadtSX81734212 = -371860972;    int VxVimadtSX85425037 = -717593392;    int VxVimadtSX88802720 = -269484061;    int VxVimadtSX91039159 = -516778010;    int VxVimadtSX91165970 = -184321358;    int VxVimadtSX54950129 = 13326348;    int VxVimadtSX2128232 = -644191162;    int VxVimadtSX73308858 = -66661342;    int VxVimadtSX70970828 = -178279091;    int VxVimadtSX58288376 = -628222155;    int VxVimadtSX69867548 = -83268008;    int VxVimadtSX50992385 = -480306871;    int VxVimadtSX35078865 = -593451052;    int VxVimadtSX37580939 = -256356085;    int VxVimadtSX17941492 = -597055900;    int VxVimadtSX51619842 = -854836014;    int VxVimadtSX93376798 = -429433683;    int VxVimadtSX83875224 = -334695269;    int VxVimadtSX98326291 = 60306897;    int VxVimadtSX40290803 = -598675653;    int VxVimadtSX94020047 = -229397855;    int VxVimadtSX46887802 = -187149830;    int VxVimadtSX81455388 = -994619978;    int VxVimadtSX10819510 = 12973085;    int VxVimadtSX6576426 = -950907714;    int VxVimadtSX73191002 = -354966565;    int VxVimadtSX27787719 = -704452213;    int VxVimadtSX76702613 = -942421027;    int VxVimadtSX76558615 = -176707430;    int VxVimadtSX17295446 = -781551237;     VxVimadtSX57380350 = VxVimadtSX39684849;     VxVimadtSX39684849 = VxVimadtSX8006166;     VxVimadtSX8006166 = VxVimadtSX62955137;     VxVimadtSX62955137 = VxVimadtSX69543035;     VxVimadtSX69543035 = VxVimadtSX10031089;     VxVimadtSX10031089 = VxVimadtSX28149948;     VxVimadtSX28149948 = VxVimadtSX60887570;     VxVimadtSX60887570 = VxVimadtSX83178784;     VxVimadtSX83178784 = VxVimadtSX17814851;     VxVimadtSX17814851 = VxVimadtSX63290132;     VxVimadtSX63290132 = VxVimadtSX75137901;     VxVimadtSX75137901 = VxVimadtSX34777974;     VxVimadtSX34777974 = VxVimadtSX97702540;     VxVimadtSX97702540 = VxVimadtSX89166985;     VxVimadtSX89166985 = VxVimadtSX94973507;     VxVimadtSX94973507 = VxVimadtSX49899372;     VxVimadtSX49899372 = VxVimadtSX39285072;     VxVimadtSX39285072 = VxVimadtSX38325656;     VxVimadtSX38325656 = VxVimadtSX91283154;     VxVimadtSX91283154 = VxVimadtSX34140823;     VxVimadtSX34140823 = VxVimadtSX99918997;     VxVimadtSX99918997 = VxVimadtSX41777937;     VxVimadtSX41777937 = VxVimadtSX41654673;     VxVimadtSX41654673 = VxVimadtSX79936191;     VxVimadtSX79936191 = VxVimadtSX29880193;     VxVimadtSX29880193 = VxVimadtSX69321018;     VxVimadtSX69321018 = VxVimadtSX1223766;     VxVimadtSX1223766 = VxVimadtSX9484596;     VxVimadtSX9484596 = VxVimadtSX62090632;     VxVimadtSX62090632 = VxVimadtSX25904387;     VxVimadtSX25904387 = VxVimadtSX9901734;     VxVimadtSX9901734 = VxVimadtSX73349064;     VxVimadtSX73349064 = VxVimadtSX28598382;     VxVimadtSX28598382 = VxVimadtSX97752156;     VxVimadtSX97752156 = VxVimadtSX91926958;     VxVimadtSX91926958 = VxVimadtSX13239296;     VxVimadtSX13239296 = VxVimadtSX64474335;     VxVimadtSX64474335 = VxVimadtSX50482352;     VxVimadtSX50482352 = VxVimadtSX47286496;     VxVimadtSX47286496 = VxVimadtSX117185;     VxVimadtSX117185 = VxVimadtSX79190693;     VxVimadtSX79190693 = VxVimadtSX97790765;     VxVimadtSX97790765 = VxVimadtSX68469079;     VxVimadtSX68469079 = VxVimadtSX70683845;     VxVimadtSX70683845 = VxVimadtSX21647816;     VxVimadtSX21647816 = VxVimadtSX60012645;     VxVimadtSX60012645 = VxVimadtSX18328633;     VxVimadtSX18328633 = VxVimadtSX66144901;     VxVimadtSX66144901 = VxVimadtSX71903656;     VxVimadtSX71903656 = VxVimadtSX44149140;     VxVimadtSX44149140 = VxVimadtSX74284545;     VxVimadtSX74284545 = VxVimadtSX16524936;     VxVimadtSX16524936 = VxVimadtSX89473840;     VxVimadtSX89473840 = VxVimadtSX30272090;     VxVimadtSX30272090 = VxVimadtSX57461353;     VxVimadtSX57461353 = VxVimadtSX97906911;     VxVimadtSX97906911 = VxVimadtSX66351493;     VxVimadtSX66351493 = VxVimadtSX83018946;     VxVimadtSX83018946 = VxVimadtSX39662842;     VxVimadtSX39662842 = VxVimadtSX40710070;     VxVimadtSX40710070 = VxVimadtSX26926182;     VxVimadtSX26926182 = VxVimadtSX51402975;     VxVimadtSX51402975 = VxVimadtSX21088153;     VxVimadtSX21088153 = VxVimadtSX91910463;     VxVimadtSX91910463 = VxVimadtSX53388399;     VxVimadtSX53388399 = VxVimadtSX1788838;     VxVimadtSX1788838 = VxVimadtSX6179592;     VxVimadtSX6179592 = VxVimadtSX99950384;     VxVimadtSX99950384 = VxVimadtSX97240026;     VxVimadtSX97240026 = VxVimadtSX81734212;     VxVimadtSX81734212 = VxVimadtSX85425037;     VxVimadtSX85425037 = VxVimadtSX88802720;     VxVimadtSX88802720 = VxVimadtSX91039159;     VxVimadtSX91039159 = VxVimadtSX91165970;     VxVimadtSX91165970 = VxVimadtSX54950129;     VxVimadtSX54950129 = VxVimadtSX2128232;     VxVimadtSX2128232 = VxVimadtSX73308858;     VxVimadtSX73308858 = VxVimadtSX70970828;     VxVimadtSX70970828 = VxVimadtSX58288376;     VxVimadtSX58288376 = VxVimadtSX69867548;     VxVimadtSX69867548 = VxVimadtSX50992385;     VxVimadtSX50992385 = VxVimadtSX35078865;     VxVimadtSX35078865 = VxVimadtSX37580939;     VxVimadtSX37580939 = VxVimadtSX17941492;     VxVimadtSX17941492 = VxVimadtSX51619842;     VxVimadtSX51619842 = VxVimadtSX93376798;     VxVimadtSX93376798 = VxVimadtSX83875224;     VxVimadtSX83875224 = VxVimadtSX98326291;     VxVimadtSX98326291 = VxVimadtSX40290803;     VxVimadtSX40290803 = VxVimadtSX94020047;     VxVimadtSX94020047 = VxVimadtSX46887802;     VxVimadtSX46887802 = VxVimadtSX81455388;     VxVimadtSX81455388 = VxVimadtSX10819510;     VxVimadtSX10819510 = VxVimadtSX6576426;     VxVimadtSX6576426 = VxVimadtSX73191002;     VxVimadtSX73191002 = VxVimadtSX27787719;     VxVimadtSX27787719 = VxVimadtSX76702613;     VxVimadtSX76702613 = VxVimadtSX76558615;     VxVimadtSX76558615 = VxVimadtSX17295446;     VxVimadtSX17295446 = VxVimadtSX57380350;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void xGrtEaBJrD1057888() {     int OnvjiqZeFO79767467 = -415955200;    int OnvjiqZeFO24032349 = -356379466;    int OnvjiqZeFO36775445 = 62506889;    int OnvjiqZeFO77961826 = -248453903;    int OnvjiqZeFO67717289 = -599193123;    int OnvjiqZeFO24914206 = -214643249;    int OnvjiqZeFO20305687 = -981687234;    int OnvjiqZeFO36521429 = -90253112;    int OnvjiqZeFO80341915 = -902809471;    int OnvjiqZeFO9578435 = 85800240;    int OnvjiqZeFO831993 = -387960060;    int OnvjiqZeFO31040851 = -980166703;    int OnvjiqZeFO1786155 = -569530014;    int OnvjiqZeFO40317293 = -155672489;    int OnvjiqZeFO25902262 = -959729477;    int OnvjiqZeFO39229420 = -394773010;    int OnvjiqZeFO99722724 = -266959332;    int OnvjiqZeFO45261488 = -199700300;    int OnvjiqZeFO79607286 = -488856;    int OnvjiqZeFO69799437 = -809437621;    int OnvjiqZeFO35023930 = -331359709;    int OnvjiqZeFO63734215 = -207716575;    int OnvjiqZeFO45071260 = 10305703;    int OnvjiqZeFO68154460 = -907737458;    int OnvjiqZeFO15476439 = -586139354;    int OnvjiqZeFO18183188 = 39416289;    int OnvjiqZeFO16410439 = -713920356;    int OnvjiqZeFO7135217 = 69360864;    int OnvjiqZeFO16377157 = -415167051;    int OnvjiqZeFO71247811 = -722571226;    int OnvjiqZeFO70195192 = -782529647;    int OnvjiqZeFO67442925 = -272389559;    int OnvjiqZeFO54387735 = -465090826;    int OnvjiqZeFO87987069 = -733697645;    int OnvjiqZeFO73981520 = -641033050;    int OnvjiqZeFO33619828 = -39015939;    int OnvjiqZeFO6679008 = -374192448;    int OnvjiqZeFO18100436 = -139711592;    int OnvjiqZeFO98790483 = -879519581;    int OnvjiqZeFO67472210 = -484832751;    int OnvjiqZeFO28874555 = -775659476;    int OnvjiqZeFO67699085 = -11107440;    int OnvjiqZeFO3340958 = 53710316;    int OnvjiqZeFO67913198 = -744316012;    int OnvjiqZeFO38667111 = -974608988;    int OnvjiqZeFO29748098 = -418587170;    int OnvjiqZeFO72310042 = 88168267;    int OnvjiqZeFO62927308 = -132561984;    int OnvjiqZeFO60604806 = -498450329;    int OnvjiqZeFO68987402 = -956574106;    int OnvjiqZeFO53018357 = -460295736;    int OnvjiqZeFO48135802 = -177196763;    int OnvjiqZeFO94839338 = -962795752;    int OnvjiqZeFO49760385 = -876732872;    int OnvjiqZeFO66033957 = -510581808;    int OnvjiqZeFO16033253 = -108238625;    int OnvjiqZeFO78961089 = -266685170;    int OnvjiqZeFO68620984 = -29755654;    int OnvjiqZeFO62485387 = -662314550;    int OnvjiqZeFO49534102 = -538609412;    int OnvjiqZeFO8503767 = -500722893;    int OnvjiqZeFO13170471 = -951048098;    int OnvjiqZeFO20144273 = -675086061;    int OnvjiqZeFO9094104 = -80238246;    int OnvjiqZeFO39383243 = -131670114;    int OnvjiqZeFO33389068 = -15570501;    int OnvjiqZeFO76653115 = -415075877;    int OnvjiqZeFO13799085 = -835832370;    int OnvjiqZeFO66335772 = -514639439;    int OnvjiqZeFO92282434 = -820713539;    int OnvjiqZeFO32550413 = 79419437;    int OnvjiqZeFO81622288 = -27247740;    int OnvjiqZeFO46471005 = -320180720;    int OnvjiqZeFO12135076 = -515656106;    int OnvjiqZeFO40924883 = 66221855;    int OnvjiqZeFO67324845 = -220252270;    int OnvjiqZeFO60393258 = -161426892;    int OnvjiqZeFO77158062 = -245378286;    int OnvjiqZeFO29487349 = -933128470;    int OnvjiqZeFO85728341 = -67552184;    int OnvjiqZeFO45873145 = 51248021;    int OnvjiqZeFO53483131 = -481358373;    int OnvjiqZeFO46530410 = -432188808;    int OnvjiqZeFO47389754 = -458592945;    int OnvjiqZeFO18229455 = -162275490;    int OnvjiqZeFO22059390 = -505332884;    int OnvjiqZeFO72603586 = -309593807;    int OnvjiqZeFO4627350 = -588357955;    int OnvjiqZeFO21953112 = -123115838;    int OnvjiqZeFO57948268 = -432794425;    int OnvjiqZeFO54658739 = -772330769;    int OnvjiqZeFO38058023 = -244436795;    int OnvjiqZeFO55615048 = -477397042;    int OnvjiqZeFO49256381 = -240910169;    int OnvjiqZeFO58968444 = -984109858;    int OnvjiqZeFO15704085 = -824611379;    int OnvjiqZeFO47554813 = -336021379;    int OnvjiqZeFO94246853 = -866051439;    int OnvjiqZeFO28529955 = -512645898;    int OnvjiqZeFO5278044 = -415955200;     OnvjiqZeFO79767467 = OnvjiqZeFO24032349;     OnvjiqZeFO24032349 = OnvjiqZeFO36775445;     OnvjiqZeFO36775445 = OnvjiqZeFO77961826;     OnvjiqZeFO77961826 = OnvjiqZeFO67717289;     OnvjiqZeFO67717289 = OnvjiqZeFO24914206;     OnvjiqZeFO24914206 = OnvjiqZeFO20305687;     OnvjiqZeFO20305687 = OnvjiqZeFO36521429;     OnvjiqZeFO36521429 = OnvjiqZeFO80341915;     OnvjiqZeFO80341915 = OnvjiqZeFO9578435;     OnvjiqZeFO9578435 = OnvjiqZeFO831993;     OnvjiqZeFO831993 = OnvjiqZeFO31040851;     OnvjiqZeFO31040851 = OnvjiqZeFO1786155;     OnvjiqZeFO1786155 = OnvjiqZeFO40317293;     OnvjiqZeFO40317293 = OnvjiqZeFO25902262;     OnvjiqZeFO25902262 = OnvjiqZeFO39229420;     OnvjiqZeFO39229420 = OnvjiqZeFO99722724;     OnvjiqZeFO99722724 = OnvjiqZeFO45261488;     OnvjiqZeFO45261488 = OnvjiqZeFO79607286;     OnvjiqZeFO79607286 = OnvjiqZeFO69799437;     OnvjiqZeFO69799437 = OnvjiqZeFO35023930;     OnvjiqZeFO35023930 = OnvjiqZeFO63734215;     OnvjiqZeFO63734215 = OnvjiqZeFO45071260;     OnvjiqZeFO45071260 = OnvjiqZeFO68154460;     OnvjiqZeFO68154460 = OnvjiqZeFO15476439;     OnvjiqZeFO15476439 = OnvjiqZeFO18183188;     OnvjiqZeFO18183188 = OnvjiqZeFO16410439;     OnvjiqZeFO16410439 = OnvjiqZeFO7135217;     OnvjiqZeFO7135217 = OnvjiqZeFO16377157;     OnvjiqZeFO16377157 = OnvjiqZeFO71247811;     OnvjiqZeFO71247811 = OnvjiqZeFO70195192;     OnvjiqZeFO70195192 = OnvjiqZeFO67442925;     OnvjiqZeFO67442925 = OnvjiqZeFO54387735;     OnvjiqZeFO54387735 = OnvjiqZeFO87987069;     OnvjiqZeFO87987069 = OnvjiqZeFO73981520;     OnvjiqZeFO73981520 = OnvjiqZeFO33619828;     OnvjiqZeFO33619828 = OnvjiqZeFO6679008;     OnvjiqZeFO6679008 = OnvjiqZeFO18100436;     OnvjiqZeFO18100436 = OnvjiqZeFO98790483;     OnvjiqZeFO98790483 = OnvjiqZeFO67472210;     OnvjiqZeFO67472210 = OnvjiqZeFO28874555;     OnvjiqZeFO28874555 = OnvjiqZeFO67699085;     OnvjiqZeFO67699085 = OnvjiqZeFO3340958;     OnvjiqZeFO3340958 = OnvjiqZeFO67913198;     OnvjiqZeFO67913198 = OnvjiqZeFO38667111;     OnvjiqZeFO38667111 = OnvjiqZeFO29748098;     OnvjiqZeFO29748098 = OnvjiqZeFO72310042;     OnvjiqZeFO72310042 = OnvjiqZeFO62927308;     OnvjiqZeFO62927308 = OnvjiqZeFO60604806;     OnvjiqZeFO60604806 = OnvjiqZeFO68987402;     OnvjiqZeFO68987402 = OnvjiqZeFO53018357;     OnvjiqZeFO53018357 = OnvjiqZeFO48135802;     OnvjiqZeFO48135802 = OnvjiqZeFO94839338;     OnvjiqZeFO94839338 = OnvjiqZeFO49760385;     OnvjiqZeFO49760385 = OnvjiqZeFO66033957;     OnvjiqZeFO66033957 = OnvjiqZeFO16033253;     OnvjiqZeFO16033253 = OnvjiqZeFO78961089;     OnvjiqZeFO78961089 = OnvjiqZeFO68620984;     OnvjiqZeFO68620984 = OnvjiqZeFO62485387;     OnvjiqZeFO62485387 = OnvjiqZeFO49534102;     OnvjiqZeFO49534102 = OnvjiqZeFO8503767;     OnvjiqZeFO8503767 = OnvjiqZeFO13170471;     OnvjiqZeFO13170471 = OnvjiqZeFO20144273;     OnvjiqZeFO20144273 = OnvjiqZeFO9094104;     OnvjiqZeFO9094104 = OnvjiqZeFO39383243;     OnvjiqZeFO39383243 = OnvjiqZeFO33389068;     OnvjiqZeFO33389068 = OnvjiqZeFO76653115;     OnvjiqZeFO76653115 = OnvjiqZeFO13799085;     OnvjiqZeFO13799085 = OnvjiqZeFO66335772;     OnvjiqZeFO66335772 = OnvjiqZeFO92282434;     OnvjiqZeFO92282434 = OnvjiqZeFO32550413;     OnvjiqZeFO32550413 = OnvjiqZeFO81622288;     OnvjiqZeFO81622288 = OnvjiqZeFO46471005;     OnvjiqZeFO46471005 = OnvjiqZeFO12135076;     OnvjiqZeFO12135076 = OnvjiqZeFO40924883;     OnvjiqZeFO40924883 = OnvjiqZeFO67324845;     OnvjiqZeFO67324845 = OnvjiqZeFO60393258;     OnvjiqZeFO60393258 = OnvjiqZeFO77158062;     OnvjiqZeFO77158062 = OnvjiqZeFO29487349;     OnvjiqZeFO29487349 = OnvjiqZeFO85728341;     OnvjiqZeFO85728341 = OnvjiqZeFO45873145;     OnvjiqZeFO45873145 = OnvjiqZeFO53483131;     OnvjiqZeFO53483131 = OnvjiqZeFO46530410;     OnvjiqZeFO46530410 = OnvjiqZeFO47389754;     OnvjiqZeFO47389754 = OnvjiqZeFO18229455;     OnvjiqZeFO18229455 = OnvjiqZeFO22059390;     OnvjiqZeFO22059390 = OnvjiqZeFO72603586;     OnvjiqZeFO72603586 = OnvjiqZeFO4627350;     OnvjiqZeFO4627350 = OnvjiqZeFO21953112;     OnvjiqZeFO21953112 = OnvjiqZeFO57948268;     OnvjiqZeFO57948268 = OnvjiqZeFO54658739;     OnvjiqZeFO54658739 = OnvjiqZeFO38058023;     OnvjiqZeFO38058023 = OnvjiqZeFO55615048;     OnvjiqZeFO55615048 = OnvjiqZeFO49256381;     OnvjiqZeFO49256381 = OnvjiqZeFO58968444;     OnvjiqZeFO58968444 = OnvjiqZeFO15704085;     OnvjiqZeFO15704085 = OnvjiqZeFO47554813;     OnvjiqZeFO47554813 = OnvjiqZeFO94246853;     OnvjiqZeFO94246853 = OnvjiqZeFO28529955;     OnvjiqZeFO28529955 = OnvjiqZeFO5278044;     OnvjiqZeFO5278044 = OnvjiqZeFO79767467;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void eVuTVkESSn37702862() {     int eZBWJzxbZv2154586 = -50359163;    int eZBWJzxbZv8379850 = -304922521;    int eZBWJzxbZv65544723 = -177146955;    int eZBWJzxbZv92968514 = -254861435;    int eZBWJzxbZv65891543 = -90250124;    int eZBWJzxbZv39797322 = -75251627;    int eZBWJzxbZv12461426 = -803674540;    int eZBWJzxbZv12155288 = -560849756;    int eZBWJzxbZv77505046 = -787005510;    int eZBWJzxbZv1342020 = -756941093;    int eZBWJzxbZv38373852 = -384584460;    int eZBWJzxbZv86943799 = -130911996;    int eZBWJzxbZv68794334 = -88247334;    int eZBWJzxbZv82932045 = -683381246;    int eZBWJzxbZv62637539 = -587207190;    int eZBWJzxbZv83485333 = -776440553;    int eZBWJzxbZv49546076 = -544239152;    int eZBWJzxbZv51237905 = -651442991;    int eZBWJzxbZv20888918 = -135590982;    int eZBWJzxbZv48315721 = 37927925;    int eZBWJzxbZv35907037 = -322847018;    int eZBWJzxbZv27549434 = -712482889;    int eZBWJzxbZv48364583 = -165774889;    int eZBWJzxbZv94654247 = -371730315;    int eZBWJzxbZv51016685 = -207698478;    int eZBWJzxbZv6486183 = -404477935;    int eZBWJzxbZv63499859 = -471506849;    int eZBWJzxbZv13046667 = -819093590;    int eZBWJzxbZv23269717 = -759424170;    int eZBWJzxbZv80404991 = -642867093;    int eZBWJzxbZv14485998 = -863845799;    int eZBWJzxbZv24984116 = -924460914;    int eZBWJzxbZv35426405 = -994434321;    int eZBWJzxbZv47375757 = 42175505;    int eZBWJzxbZv50210885 = -704789472;    int eZBWJzxbZv75312697 = -354411315;    int eZBWJzxbZv118720 = -107140399;    int eZBWJzxbZv71726536 = -7337064;    int eZBWJzxbZv47098614 = -180565612;    int eZBWJzxbZv87657925 = -621056781;    int eZBWJzxbZv57631926 = -178837142;    int eZBWJzxbZv56207476 = -869016131;    int eZBWJzxbZv8891150 = -933820267;    int eZBWJzxbZv67357317 = -741679661;    int eZBWJzxbZv6650378 = -783752466;    int eZBWJzxbZv37848379 = -600816266;    int eZBWJzxbZv84607440 = -490241985;    int eZBWJzxbZv7525983 = -989096975;    int eZBWJzxbZv55064711 = -448167028;    int eZBWJzxbZv66071147 = 1405635;    int eZBWJzxbZv61887573 = -815372013;    int eZBWJzxbZv21987060 = -608016045;    int eZBWJzxbZv73153741 = -634706982;    int eZBWJzxbZv10046931 = -52413681;    int eZBWJzxbZv1795825 = -651285923;    int eZBWJzxbZv74605151 = -337876275;    int eZBWJzxbZv60015266 = -39147633;    int eZBWJzxbZv70890475 = -805416641;    int eZBWJzxbZv41951829 = 52837043;    int eZBWJzxbZv59405361 = -685772190;    int eZBWJzxbZv76297463 = -603744779;    int eZBWJzxbZv99414759 = -984580951;    int eZBWJzxbZv88885570 = -801425586;    int eZBWJzxbZv97100055 = -44138418;    int eZBWJzxbZv86856022 = -893095295;    int eZBWJzxbZv13389736 = -460123547;    int eZBWJzxbZv51517394 = -136477676;    int eZBWJzxbZv21418578 = -30422839;    int eZBWJzxbZv32721160 = -978591774;    int eZBWJzxbZv87324842 = -132795876;    int eZBWJzxbZv83366613 = -569300154;    int eZBWJzxbZv77819539 = -436902088;    int eZBWJzxbZv4139291 = -370877379;    int eZBWJzxbZv33230992 = -514534202;    int eZBWJzxbZv90683794 = -783234933;    int eZBWJzxbZv79699560 = -453830887;    int eZBWJzxbZv18658284 = -778662622;    int eZBWJzxbZv81007266 = -424095229;    int eZBWJzxbZv88003869 = -587977850;    int eZBWJzxbZv13168306 = -606882213;    int eZBWJzxbZv21878743 = -914235950;    int eZBWJzxbZv55973877 = -482409875;    int eZBWJzxbZv57981955 = -270926563;    int eZBWJzxbZv57198569 = -660829805;    int eZBWJzxbZv18517418 = -827495080;    int eZBWJzxbZv92498938 = -155829754;    int eZBWJzxbZv51830374 = -189753932;    int eZBWJzxbZv25379475 = -842020641;    int eZBWJzxbZv45579932 = -306538573;    int eZBWJzxbZv75605734 = -266913198;    int eZBWJzxbZv15297431 = -215263683;    int eZBWJzxbZv29228244 = -301723759;    int eZBWJzxbZv29774708 = 39825893;    int eZBWJzxbZv87693253 = -494793423;    int eZBWJzxbZv11360462 = 82687997;    int eZBWJzxbZv58217167 = -194256192;    int eZBWJzxbZv67321906 = 32409455;    int eZBWJzxbZv11791095 = -789681850;    int eZBWJzxbZv80501295 = -848584366;    int eZBWJzxbZv93260641 = -50359163;     eZBWJzxbZv2154586 = eZBWJzxbZv8379850;     eZBWJzxbZv8379850 = eZBWJzxbZv65544723;     eZBWJzxbZv65544723 = eZBWJzxbZv92968514;     eZBWJzxbZv92968514 = eZBWJzxbZv65891543;     eZBWJzxbZv65891543 = eZBWJzxbZv39797322;     eZBWJzxbZv39797322 = eZBWJzxbZv12461426;     eZBWJzxbZv12461426 = eZBWJzxbZv12155288;     eZBWJzxbZv12155288 = eZBWJzxbZv77505046;     eZBWJzxbZv77505046 = eZBWJzxbZv1342020;     eZBWJzxbZv1342020 = eZBWJzxbZv38373852;     eZBWJzxbZv38373852 = eZBWJzxbZv86943799;     eZBWJzxbZv86943799 = eZBWJzxbZv68794334;     eZBWJzxbZv68794334 = eZBWJzxbZv82932045;     eZBWJzxbZv82932045 = eZBWJzxbZv62637539;     eZBWJzxbZv62637539 = eZBWJzxbZv83485333;     eZBWJzxbZv83485333 = eZBWJzxbZv49546076;     eZBWJzxbZv49546076 = eZBWJzxbZv51237905;     eZBWJzxbZv51237905 = eZBWJzxbZv20888918;     eZBWJzxbZv20888918 = eZBWJzxbZv48315721;     eZBWJzxbZv48315721 = eZBWJzxbZv35907037;     eZBWJzxbZv35907037 = eZBWJzxbZv27549434;     eZBWJzxbZv27549434 = eZBWJzxbZv48364583;     eZBWJzxbZv48364583 = eZBWJzxbZv94654247;     eZBWJzxbZv94654247 = eZBWJzxbZv51016685;     eZBWJzxbZv51016685 = eZBWJzxbZv6486183;     eZBWJzxbZv6486183 = eZBWJzxbZv63499859;     eZBWJzxbZv63499859 = eZBWJzxbZv13046667;     eZBWJzxbZv13046667 = eZBWJzxbZv23269717;     eZBWJzxbZv23269717 = eZBWJzxbZv80404991;     eZBWJzxbZv80404991 = eZBWJzxbZv14485998;     eZBWJzxbZv14485998 = eZBWJzxbZv24984116;     eZBWJzxbZv24984116 = eZBWJzxbZv35426405;     eZBWJzxbZv35426405 = eZBWJzxbZv47375757;     eZBWJzxbZv47375757 = eZBWJzxbZv50210885;     eZBWJzxbZv50210885 = eZBWJzxbZv75312697;     eZBWJzxbZv75312697 = eZBWJzxbZv118720;     eZBWJzxbZv118720 = eZBWJzxbZv71726536;     eZBWJzxbZv71726536 = eZBWJzxbZv47098614;     eZBWJzxbZv47098614 = eZBWJzxbZv87657925;     eZBWJzxbZv87657925 = eZBWJzxbZv57631926;     eZBWJzxbZv57631926 = eZBWJzxbZv56207476;     eZBWJzxbZv56207476 = eZBWJzxbZv8891150;     eZBWJzxbZv8891150 = eZBWJzxbZv67357317;     eZBWJzxbZv67357317 = eZBWJzxbZv6650378;     eZBWJzxbZv6650378 = eZBWJzxbZv37848379;     eZBWJzxbZv37848379 = eZBWJzxbZv84607440;     eZBWJzxbZv84607440 = eZBWJzxbZv7525983;     eZBWJzxbZv7525983 = eZBWJzxbZv55064711;     eZBWJzxbZv55064711 = eZBWJzxbZv66071147;     eZBWJzxbZv66071147 = eZBWJzxbZv61887573;     eZBWJzxbZv61887573 = eZBWJzxbZv21987060;     eZBWJzxbZv21987060 = eZBWJzxbZv73153741;     eZBWJzxbZv73153741 = eZBWJzxbZv10046931;     eZBWJzxbZv10046931 = eZBWJzxbZv1795825;     eZBWJzxbZv1795825 = eZBWJzxbZv74605151;     eZBWJzxbZv74605151 = eZBWJzxbZv60015266;     eZBWJzxbZv60015266 = eZBWJzxbZv70890475;     eZBWJzxbZv70890475 = eZBWJzxbZv41951829;     eZBWJzxbZv41951829 = eZBWJzxbZv59405361;     eZBWJzxbZv59405361 = eZBWJzxbZv76297463;     eZBWJzxbZv76297463 = eZBWJzxbZv99414759;     eZBWJzxbZv99414759 = eZBWJzxbZv88885570;     eZBWJzxbZv88885570 = eZBWJzxbZv97100055;     eZBWJzxbZv97100055 = eZBWJzxbZv86856022;     eZBWJzxbZv86856022 = eZBWJzxbZv13389736;     eZBWJzxbZv13389736 = eZBWJzxbZv51517394;     eZBWJzxbZv51517394 = eZBWJzxbZv21418578;     eZBWJzxbZv21418578 = eZBWJzxbZv32721160;     eZBWJzxbZv32721160 = eZBWJzxbZv87324842;     eZBWJzxbZv87324842 = eZBWJzxbZv83366613;     eZBWJzxbZv83366613 = eZBWJzxbZv77819539;     eZBWJzxbZv77819539 = eZBWJzxbZv4139291;     eZBWJzxbZv4139291 = eZBWJzxbZv33230992;     eZBWJzxbZv33230992 = eZBWJzxbZv90683794;     eZBWJzxbZv90683794 = eZBWJzxbZv79699560;     eZBWJzxbZv79699560 = eZBWJzxbZv18658284;     eZBWJzxbZv18658284 = eZBWJzxbZv81007266;     eZBWJzxbZv81007266 = eZBWJzxbZv88003869;     eZBWJzxbZv88003869 = eZBWJzxbZv13168306;     eZBWJzxbZv13168306 = eZBWJzxbZv21878743;     eZBWJzxbZv21878743 = eZBWJzxbZv55973877;     eZBWJzxbZv55973877 = eZBWJzxbZv57981955;     eZBWJzxbZv57981955 = eZBWJzxbZv57198569;     eZBWJzxbZv57198569 = eZBWJzxbZv18517418;     eZBWJzxbZv18517418 = eZBWJzxbZv92498938;     eZBWJzxbZv92498938 = eZBWJzxbZv51830374;     eZBWJzxbZv51830374 = eZBWJzxbZv25379475;     eZBWJzxbZv25379475 = eZBWJzxbZv45579932;     eZBWJzxbZv45579932 = eZBWJzxbZv75605734;     eZBWJzxbZv75605734 = eZBWJzxbZv15297431;     eZBWJzxbZv15297431 = eZBWJzxbZv29228244;     eZBWJzxbZv29228244 = eZBWJzxbZv29774708;     eZBWJzxbZv29774708 = eZBWJzxbZv87693253;     eZBWJzxbZv87693253 = eZBWJzxbZv11360462;     eZBWJzxbZv11360462 = eZBWJzxbZv58217167;     eZBWJzxbZv58217167 = eZBWJzxbZv67321906;     eZBWJzxbZv67321906 = eZBWJzxbZv11791095;     eZBWJzxbZv11791095 = eZBWJzxbZv80501295;     eZBWJzxbZv80501295 = eZBWJzxbZv93260641;     eZBWJzxbZv93260641 = eZBWJzxbZv2154586;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void NLMKqvFWti54499165() {     int vKdMamdYSo69604857 = -264141227;    int vKdMamdYSo34289882 = -990928661;    int vKdMamdYSo92060351 = -485198030;    int vKdMamdYSo94724490 = -904354878;    int vKdMamdYSo20594166 = -651968847;    int vKdMamdYSo45073642 = -432176559;    int vKdMamdYSo1948884 = -444434583;    int vKdMamdYSo64228428 = -849670270;    int vKdMamdYSo36574022 = -675147868;    int vKdMamdYSo95233439 = -834619689;    int vKdMamdYSo13991621 = -85418940;    int vKdMamdYSo60221502 = -121451357;    int vKdMamdYSo54975581 = -313082584;    int vKdMamdYSo94756224 = -110170445;    int vKdMamdYSo48410002 = -457245375;    int vKdMamdYSo39831116 = 91701970;    int vKdMamdYSo21350518 = -952191592;    int vKdMamdYSo17892178 = -224610328;    int vKdMamdYSo42627744 = -503898325;    int vKdMamdYSo60494128 = 11354317;    int vKdMamdYSo30751345 = -716546872;    int vKdMamdYSo40405443 = -833797520;    int vKdMamdYSo68277115 = -545085441;    int vKdMamdYSo75114583 = -609735360;    int vKdMamdYSo59999526 = -701013075;    int vKdMamdYSo54184659 = -236309027;    int vKdMamdYSo21572947 = -108857838;    int vKdMamdYSo80543304 = -87493073;    int vKdMamdYSo60766777 = -934780813;    int vKdMamdYSo63456710 = 49635845;    int vKdMamdYSo60508504 = -8495222;    int vKdMamdYSo82208763 = -726944478;    int vKdMamdYSo20949596 = -573890396;    int vKdMamdYSo18683766 = -277676924;    int vKdMamdYSo44238517 = -43713725;    int vKdMamdYSo93621069 = -821321397;    int vKdMamdYSo43675216 = -147996303;    int vKdMamdYSo37531279 = -907992158;    int vKdMamdYSo50673254 = -985342667;    int vKdMamdYSo80099710 = -159634696;    int vKdMamdYSo93239133 = 15054554;    int vKdMamdYSo22038099 = 15828571;    int vKdMamdYSo94349762 = 40936351;    int vKdMamdYSo82132407 = -121084904;    int vKdMamdYSo20615854 = -649150922;    int vKdMamdYSo6168159 = -474857314;    int vKdMamdYSo37116483 = -292893488;    int vKdMamdYSo97317886 = -758960092;    int vKdMamdYSo63483943 = -615853399;    int vKdMamdYSo27795096 = -369112793;    int vKdMamdYSo35439552 = -257639313;    int vKdMamdYSo17724871 = -471048744;    int vKdMamdYSo76194372 = -162544710;    int vKdMamdYSo96787006 = -389872130;    int vKdMamdYSo23747820 = -306290174;    int vKdMamdYSo29199414 = -430343708;    int vKdMamdYSo66012767 = -345843220;    int vKdMamdYSo16945768 = -875462670;    int vKdMamdYSo34724964 = -103341803;    int vKdMamdYSo66409506 = -315659820;    int vKdMamdYSo23500695 = -223318721;    int vKdMamdYSo21405580 = -256941511;    int vKdMamdYSo3461651 = -914889457;    int vKdMamdYSo73117311 = -624783714;    int vKdMamdYSo34724936 = -726124467;    int vKdMamdYSo31782858 = -358474462;    int vKdMamdYSo39271906 = -547560962;    int vKdMamdYSo36291815 = 64594340;    int vKdMamdYSo50517707 = 33543280;    int vKdMamdYSo54788932 = -635923978;    int vKdMamdYSo96155900 = -760301728;    int vKdMamdYSo83819238 = 55800565;    int vKdMamdYSo67218923 = -239267661;    int vKdMamdYSo62528033 = -244263629;    int vKdMamdYSo67254995 = 96299762;    int vKdMamdYSo8713247 = -632375444;    int vKdMamdYSo46055681 = -774733871;    int vKdMamdYSo86144708 = -324000538;    int vKdMamdYSo54498730 = -960584439;    int vKdMamdYSo53831368 = -126155761;    int vKdMamdYSo17068177 = -943415539;    int vKdMamdYSo24255061 = -349897747;    int vKdMamdYSo17059361 = -471639674;    int vKdMamdYSo32971681 = -465668021;    int vKdMamdYSo28017159 = -692724842;    int vKdMamdYSo42783634 = -537446479;    int vKdMamdYSo6014392 = -464399769;    int vKdMamdYSo24162590 = -84018266;    int vKdMamdYSo94935946 = -971386751;    int vKdMamdYSo15039104 = -613370018;    int vKdMamdYSo27608303 = -375478177;    int vKdMamdYSo26729448 = -272533633;    int vKdMamdYSo2806315 = -704650355;    int vKdMamdYSo84263748 = -569682847;    int vKdMamdYSo56599016 = -936315976;    int vKdMamdYSo71833554 = -728003936;    int vKdMamdYSo18576448 = -69281972;    int vKdMamdYSo21232452 = -334279936;    int vKdMamdYSo47407472 = -394960438;    int vKdMamdYSo88832996 = -264141227;     vKdMamdYSo69604857 = vKdMamdYSo34289882;     vKdMamdYSo34289882 = vKdMamdYSo92060351;     vKdMamdYSo92060351 = vKdMamdYSo94724490;     vKdMamdYSo94724490 = vKdMamdYSo20594166;     vKdMamdYSo20594166 = vKdMamdYSo45073642;     vKdMamdYSo45073642 = vKdMamdYSo1948884;     vKdMamdYSo1948884 = vKdMamdYSo64228428;     vKdMamdYSo64228428 = vKdMamdYSo36574022;     vKdMamdYSo36574022 = vKdMamdYSo95233439;     vKdMamdYSo95233439 = vKdMamdYSo13991621;     vKdMamdYSo13991621 = vKdMamdYSo60221502;     vKdMamdYSo60221502 = vKdMamdYSo54975581;     vKdMamdYSo54975581 = vKdMamdYSo94756224;     vKdMamdYSo94756224 = vKdMamdYSo48410002;     vKdMamdYSo48410002 = vKdMamdYSo39831116;     vKdMamdYSo39831116 = vKdMamdYSo21350518;     vKdMamdYSo21350518 = vKdMamdYSo17892178;     vKdMamdYSo17892178 = vKdMamdYSo42627744;     vKdMamdYSo42627744 = vKdMamdYSo60494128;     vKdMamdYSo60494128 = vKdMamdYSo30751345;     vKdMamdYSo30751345 = vKdMamdYSo40405443;     vKdMamdYSo40405443 = vKdMamdYSo68277115;     vKdMamdYSo68277115 = vKdMamdYSo75114583;     vKdMamdYSo75114583 = vKdMamdYSo59999526;     vKdMamdYSo59999526 = vKdMamdYSo54184659;     vKdMamdYSo54184659 = vKdMamdYSo21572947;     vKdMamdYSo21572947 = vKdMamdYSo80543304;     vKdMamdYSo80543304 = vKdMamdYSo60766777;     vKdMamdYSo60766777 = vKdMamdYSo63456710;     vKdMamdYSo63456710 = vKdMamdYSo60508504;     vKdMamdYSo60508504 = vKdMamdYSo82208763;     vKdMamdYSo82208763 = vKdMamdYSo20949596;     vKdMamdYSo20949596 = vKdMamdYSo18683766;     vKdMamdYSo18683766 = vKdMamdYSo44238517;     vKdMamdYSo44238517 = vKdMamdYSo93621069;     vKdMamdYSo93621069 = vKdMamdYSo43675216;     vKdMamdYSo43675216 = vKdMamdYSo37531279;     vKdMamdYSo37531279 = vKdMamdYSo50673254;     vKdMamdYSo50673254 = vKdMamdYSo80099710;     vKdMamdYSo80099710 = vKdMamdYSo93239133;     vKdMamdYSo93239133 = vKdMamdYSo22038099;     vKdMamdYSo22038099 = vKdMamdYSo94349762;     vKdMamdYSo94349762 = vKdMamdYSo82132407;     vKdMamdYSo82132407 = vKdMamdYSo20615854;     vKdMamdYSo20615854 = vKdMamdYSo6168159;     vKdMamdYSo6168159 = vKdMamdYSo37116483;     vKdMamdYSo37116483 = vKdMamdYSo97317886;     vKdMamdYSo97317886 = vKdMamdYSo63483943;     vKdMamdYSo63483943 = vKdMamdYSo27795096;     vKdMamdYSo27795096 = vKdMamdYSo35439552;     vKdMamdYSo35439552 = vKdMamdYSo17724871;     vKdMamdYSo17724871 = vKdMamdYSo76194372;     vKdMamdYSo76194372 = vKdMamdYSo96787006;     vKdMamdYSo96787006 = vKdMamdYSo23747820;     vKdMamdYSo23747820 = vKdMamdYSo29199414;     vKdMamdYSo29199414 = vKdMamdYSo66012767;     vKdMamdYSo66012767 = vKdMamdYSo16945768;     vKdMamdYSo16945768 = vKdMamdYSo34724964;     vKdMamdYSo34724964 = vKdMamdYSo66409506;     vKdMamdYSo66409506 = vKdMamdYSo23500695;     vKdMamdYSo23500695 = vKdMamdYSo21405580;     vKdMamdYSo21405580 = vKdMamdYSo3461651;     vKdMamdYSo3461651 = vKdMamdYSo73117311;     vKdMamdYSo73117311 = vKdMamdYSo34724936;     vKdMamdYSo34724936 = vKdMamdYSo31782858;     vKdMamdYSo31782858 = vKdMamdYSo39271906;     vKdMamdYSo39271906 = vKdMamdYSo36291815;     vKdMamdYSo36291815 = vKdMamdYSo50517707;     vKdMamdYSo50517707 = vKdMamdYSo54788932;     vKdMamdYSo54788932 = vKdMamdYSo96155900;     vKdMamdYSo96155900 = vKdMamdYSo83819238;     vKdMamdYSo83819238 = vKdMamdYSo67218923;     vKdMamdYSo67218923 = vKdMamdYSo62528033;     vKdMamdYSo62528033 = vKdMamdYSo67254995;     vKdMamdYSo67254995 = vKdMamdYSo8713247;     vKdMamdYSo8713247 = vKdMamdYSo46055681;     vKdMamdYSo46055681 = vKdMamdYSo86144708;     vKdMamdYSo86144708 = vKdMamdYSo54498730;     vKdMamdYSo54498730 = vKdMamdYSo53831368;     vKdMamdYSo53831368 = vKdMamdYSo17068177;     vKdMamdYSo17068177 = vKdMamdYSo24255061;     vKdMamdYSo24255061 = vKdMamdYSo17059361;     vKdMamdYSo17059361 = vKdMamdYSo32971681;     vKdMamdYSo32971681 = vKdMamdYSo28017159;     vKdMamdYSo28017159 = vKdMamdYSo42783634;     vKdMamdYSo42783634 = vKdMamdYSo6014392;     vKdMamdYSo6014392 = vKdMamdYSo24162590;     vKdMamdYSo24162590 = vKdMamdYSo94935946;     vKdMamdYSo94935946 = vKdMamdYSo15039104;     vKdMamdYSo15039104 = vKdMamdYSo27608303;     vKdMamdYSo27608303 = vKdMamdYSo26729448;     vKdMamdYSo26729448 = vKdMamdYSo2806315;     vKdMamdYSo2806315 = vKdMamdYSo84263748;     vKdMamdYSo84263748 = vKdMamdYSo56599016;     vKdMamdYSo56599016 = vKdMamdYSo71833554;     vKdMamdYSo71833554 = vKdMamdYSo18576448;     vKdMamdYSo18576448 = vKdMamdYSo21232452;     vKdMamdYSo21232452 = vKdMamdYSo47407472;     vKdMamdYSo47407472 = vKdMamdYSo88832996;     vKdMamdYSo88832996 = vKdMamdYSo69604857;}
// Junk Finished
