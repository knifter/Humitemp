// Sohow an menu item with current value
#include "gui.h"

#include <lvgl.h>

#include "pid.h"
#include "config.h"
#include "tools-log.h"
#include "globals.h"
#include "tools-keys.h"

#include "screens.h"

// LVGL Callback funcs
void lv_disp_cb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p);
#ifdef GUI_TOUCH
void lv_touchpad_cb(lv_indev_drv_t * indev, lv_indev_data_t * data);
#endif
#ifdef GUI_KEYPAD
uint32_t scan_keys();
void lv_keys_cb(lv_indev_drv_t * indev, lv_indev_data_t * data);
#endif

bool GUI::begin()
{
    gfx.init();
    gfx.setRotation(1);
    gfx.setColorDepth(24);

	lv_init();

    lv_disp_draw_buf_init(&_lv_draw_buf, _lv_color_buf, NULL, LV_BUF_SIZE);

    lv_disp_drv_init(&_lv_display_drv);          /*Basic initialization*/
    _lv_display_drv.flush_cb = lv_disp_cb;    /*Set your driver function*/
    _lv_display_drv.draw_buf = &_lv_draw_buf;        /*Assign the buffer to the display*/
    _lv_display_drv.hor_res = DISPLAY_WIDTH;   /*Set the horizontal resolution of the display*/
    _lv_display_drv.ver_res = DISPLAY_HEIGHT;   /*Set the vertical resolution of the display*/
    lv_disp_drv_register(&_lv_display_drv);      /*Finally register the driver*/

#ifdef GUI_TOUCH
    uint16_t calData[] = { 239, 3926, 233, 265, 3856, 3896, 3714, 308};
    gfx.setTouchCalibrate(calData);

    lv_indev_drv_init(&_lv_touch_drv);             /*Basic initialization*/
    _lv_touch_drv.type = LV_INDEV_TYPE_POINTER;    /*Touch pad is a pointer-like device*/
    _lv_touch_drv.read_cb = lv_touchpad_cb;      /*Set your driver function*/
    lv_indev_drv_register(&_lv_touch_drv);         /*Finally register the driver*/
#endif

#ifdef GUI_KEYPAD
    lv_indev_drv_init(&_lv_keys_drv);             /*Basic initialization*/
    _lv_keys_drv.type = LV_INDEV_TYPE_KEYPAD;    /*Touch pad is a pointer-like device*/
    _lv_keys_drv.read_cb = lv_keys_cb;      /*Set your driver function*/
    lv_indev_drv_register(&_lv_touch_drv);         /*Finally register the driver*/
#endif // GUI_KEYPAD

	// Empty activity stack
	while(!_scrstack.empty())
		_scrstack.pop();

	// And put bootscreen on the bottom
	pushScreen(ScreenType::BOOT);

	return true;
};

time_t GUI::loop()
{
    // TODO: Use LVGL to handle global keys/events
	// if(handle_global_keys(_keypad))
	// 	_event = KEY_NONE;

	// ScreenStack may not be empty
	if(_scrstack.size() == 0)
	{
		ERROR("actstack empty, push(BOOT)");
		pushScreen(ScreenType::BOOT);
	};

	// Keeping this (smart) ptr here is important! It prevents pop() from deleting the 
	// Activity while in handle(). It will be deleted when scr goes out of scope too
	ScreenPtr scr = _scrstack.top();

	// Debug activity
#ifdef GUI_DEBUG
	static ActivityPtr prev_act = nullptr;
	if(act != prev_act)
	{
		if(prev_act == nullptr)
			DBG("GUI: <null> -> %s", act->name());
		else
			DBG("GUI: %s -> %s", prev_act->name(), act->name());
		prev_act = act;
	};
#endif

    scr->loop();

	// LVGL ticker
    time_t now = millis();
	{
    	lv_tick_inc(now - _prv_tick);
    	_prv_tick = now;
	};
    return lv_timer_handler();
};

ScreenPtr GUI::pushScreen(ScreenType scrtype, void* data)
{
	ScreenPtr scr = NULL;
	switch(scrtype)
	{
		case ScreenType::BOOT:			scr = std::make_shared<BootScreen>(); break;
		case ScreenType::MAIN:			scr = std::make_shared<MainScreen>(); break;
		case ScreenType::MENU:			scr = std::make_shared<MenuScreen>(); break;

		default:
			showMessage("ERROR", "Invalid <ScreenType> identifier!"); 
			return NULL;
	};
    pushScreen(scr, data);
	return scr;
};

ScreenPtr GUI::pushScreen(ScreenPtr scr, void* data)
{
	DBG("GUI: Push(%s)", scr->name());
	_scrstack.push(scr);
    scr->load();

	return scr;
};

void GUI::showMessage(const char* title, const char* text)
{
    static const char * btns[] ={"Close", ""};

    //lv_layer_top()
    lv_obj_t * mbox1 = lv_msgbox_create(NULL, title, text, btns, false);
    // lv_obj_add_event_cb(mbox1, event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    // lv_obj_t *but = lv_msgbox_get_btns(mbox1);
    lv_obj_center(mbox1);
};

void GUI::popScreen(Screen* scr)
{
	// There must an activity on the stack to do this..
	if(_scrstack.empty())
		return;

	// ActivityPtr is a smart ptr. It will delete a in GUI::handle() eventually
	ScreenPtr top = _scrstack.top();
	_scrstack.pop();
	DBG("GUI: pop(%s)", top->name());
    
    // Just a check for now
    if(scr != nullptr && top.get() != scr)
    {
        ERROR("Screen* given does not match top().");
        showMessage("ERROR", "Screen* != top()");
        return;
    };

    // make the screen below active again
    _scrstack.top()->load();

	// DBG("popped, will delete (eventually): %s(%p)", a->name(), a);
	return;
};


#ifdef GUI_DEBUG
void GUI::draw_debug()
{	

};
#endif // GUI_DEBUG

void lv_disp_cb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p)
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );

    gfx.startWrite();
    gfx.setAddrWindow( area->x1, area->y1, w, h );
    //gfx.pushColors( ( uint16_t * )&color_p->full, w * h, true );
    gfx.writePixelsDMA((lgfx::rgb565_t *)&color_p->full, w * h);
    gfx.endWrite();

    lv_disp_flush_ready( disp );
};

#ifdef GUI_TOUCH
void lv_touchpad_cb(lv_indev_drv_t * indev, lv_indev_data_t * data)
{
    // Point p = M5.Touch.getPressPoint();
    // if(p.x == -1 && p.y == -1)
    // {
    //     data->state = LV_INDEV_STATE_RELEASED;
    //     return;
    // };
    // data->point.x = p.x;
    // data->point.y = p.y;
    // data->state = LV_INDEV_STATE_PRESSED;

    uint16_t touchX, touchY;
    if(gfx.getTouch( &touchX, &touchY))
    {
        data->state = LV_INDEV_STATE_PR;

        /*Set the coordinates*/
        data->point.x = touchX;
        data->point.y = touchY;

        Serial.print( "Data x " );
        Serial.println( touchX );

        Serial.print( "Data y " );
        Serial.println( touchY );
    };
    data->state = LV_INDEV_STATE_REL;
};
#endif

#ifdef GUI_KEYPAD
uint32_t scan_keys()
{
	// Read current states
	uint32_t pressed = KEY_NONE;
	if(digitalRead(PIN_BTN_A) == LOW)
		pressed |= KEY_A;
	if(digitalRead(PIN_BTN_B) == LOW)
		pressed |= KEY_B;
	if(digitalRead(PIN_BTN_C) == LOW)
		pressed |= KEY_C;
	// if(digitalRead(PIN_POWERINT) == LOW)
	// 	pressed |= KEY_P;
	return keytool_get_event(pressed);
};

void lv_keys_cb(lv_indev_drv_t * indev, lv_indev_data_t * data)
{
    data->key = scan_keys();//last_key();            /*Get the last pressed or released key*/
    if(data->key != KEY_NONE) 
        data->state = LV_INDEV_STATE_PRESSED;
    else 
        data->state = LV_INDEV_STATE_RELEASED;
	return;
};
#endif // GUI_KEYPAD
