/*	Graphics Library of Views (GLV) - GUI Building Toolkit
	See COPYRIGHT file for authors and license information */

#include <string>
#include <string.h>
#include "glv_textview.h"

namespace glv{

#define CTOR_LIST mSize(1), mAlignX(0), mAlignY(0), mVertical(false)
#define CTOR_BODY\
	disable(CropSelf | DrawBack | DrawBorder | HitTest);\
	label(str);\
	vertical(vert);

Label::Label(const std::string& str, const Spec& s)
:	View(0,0,0,0), mAlignX(0), mAlignY(0)
{
	disable(CropSelf | DrawBack | DrawBorder | HitTest);
	label(str);
	vertical(s.vert);
	size(s.size);
	pos(s.posAnch, s.dx, s.dy).anchor(s.posAnch);
}

Label::Label(const std::string& str, bool vert)
:	View(0,0,0,0), CTOR_LIST
{	CTOR_BODY }

Label::Label(const std::string& str, space_t l, space_t t, bool vert)
:	View(l,t,0,0), CTOR_LIST
{	CTOR_BODY }

Label::Label(const std::string& str, Place::t p, space_t px, space_t py, bool vert)
:	View(0,0,0,0), CTOR_LIST
{	CTOR_BODY 
	pos(p, px, py).anchor(p);
}

#undef CTOR_LIST
#undef CTOR_BODY

Label& Label::align(float vx, float vy){ mAlignX=vx; mAlignY=vy; return *this; }

Label& Label::label(const std::string& s){
	mLabel = s;
	fitExtent();
	if(numObservers(Update::Value)) notify(Update::Value, LabelChange(s));
	return *this;
}

Label& Label::size(float pixels){ mSize = pixels/8.; fitExtent(); return *this; }

Label& Label::value(const std::string& s){ return label(s); }

Label& Label::vertical(bool v){
	if(v != mVertical){
		rotateRect();
		mVertical = v;
	}
	return *this;
}

void Label::onDraw(){
	using namespace glv::draw;
	lineWidth(1);
	color(colors().text);
	if(mVertical){ translate(0,h); rotate(0,0,-90); }
	scale(mSize, mSize);
	text(mLabel.c_str());
}


void Label::fitExtent(){
	space_t dx = 8;
	space_t tw = 0, th = 8, mw = 0;
	const char * c = mLabel.c_str();

	while(*c){
		switch(*c++){
		case '\n': th += dx*2; tw = 0; break;
		case '\t': tw = ((int)(tw/32) + 1) * 32; if(tw > mw) mw=tw; break;
		case '\b': tw -= dx; break;
		default: tw += dx; if(tw > mw) mw=tw;
		}
	}
	
	space_t dw = mw*mSize - w;
	space_t dh = th*mSize - h;
	posAdd(-dw*mAlignX, -dh*mAlignY);
	
	extent(mw*mSize, th*mSize);
	if(mVertical) rotateRect();
}

void Label::rotateRect(){ t += w - h; transpose(); }


#define CTOR_LIST mNI(0), mNF(0), mVal(0), mPad(2), mAcc(0), mShowSign(true)
#define CTOR_BODY\
	resize(numInt, numFrac);\
	dig(mNI);

NumberDialer::NumberDialer(const Rect& r, int numInt, int numFrac)
:	View(r), CTOR_LIST
{	
	CTOR_BODY
	mMax = maxVal();
	mMin =-mMax;
	valSet(mVal);
}

NumberDialer::NumberDialer(const Rect& r, int numInt, int numFrac, double max, double min)
:	View(r), CTOR_LIST
{	
	CTOR_BODY
	range(max, min);	
}

NumberDialer::NumberDialer(space_t h, space_t l, space_t t, int numInt, int numFrac, double max, double min)
:	View(Rect(l,t, (h-2)*(numInt+numFrac+1), h)), CTOR_LIST
{
	CTOR_BODY
	range(max, min);
}

NumberDialer::NumberDialer(int numInt, int numFrac, double max, double min)
:	View(Rect(0,0, (12-2)*(numInt+numFrac+1), 12)), CTOR_LIST
{
	CTOR_BODY
	range(max, min);
} 

NumberDialer::NumberDialer(const NumberDialer& v)
:	View(v), CTOR_LIST
{
	dig(v.sizeInteger());
	resize(v.sizeInteger(), v.sizeFraction());
	range(v.max(), v.min());
}

#undef CTOR_LIST
#undef CTOR_BODY

double NumberDialer::max() const { return mMax*mValMul; }
double NumberDialer::min() const { return mMin*mValMul; }
int NumberDialer::sizeFraction() const { return mNF; }
int NumberDialer::sizeInteger() const { return mNI; }

NumberDialer& NumberDialer::padding(space_t v){ mPad=v; return *this; }

static void sort(double& v1, double& v2){ if(v1>v2){ double t=v1; v1=v2; v2=t; } }

NumberDialer& NumberDialer::range(double max, double min){
	sort(min,max);
	mMin = convert(min);
	mMax = convert(max);
	int m = maxVal();	// do not allow numbers larger than can be displayed
	if(mMin<-m) mMin=-m;
	if(mMax> m) mMax= m;
	showSign(min < 0);
	valSet(mVal);
	return *this;
}

NumberDialer& NumberDialer::resize(int numInt, int numFrac){
	mNI = numInt; mNF = numFrac;
	mValMul = 1. / pow(10., mNF);
	setWidth();
	return *this;
}

NumberDialer& NumberDialer::showSign(bool v){
	mShowSign=v;
	setWidth();
	return *this;
}

double NumberDialer::value() const{ return mVal * mValMul; }
NumberDialer& NumberDialer::value(double v){ valSet(convert(v)); return *this; }

void NumberDialer::onDraw(){ //printf("% g\n", value());
	using namespace glv::draw;
	float dx = w/size(); // # pixels per cell
	lineWidth(1);
	
	// draw box at position (only if focused)
	if(enabled(Focused)){
		color(colors().fore, colors().fore.a*0.4);
		rect(dig()*dx, 0, (dig()+1)*dx, h);
	}
	
	// draw number
	int absVal = mVal < 0 ? -mVal : mVal;
	int msd = mNF;	// position from right of most significant digit
	
	if(absVal > 0){
		msd = (int)log10(absVal);
		int p = size() - (mShowSign ? 2:1);
		msd = msd < mNF ? mNF : (msd > p ? p : msd);
	}
	
	if(mNI == 0) msd-=1;

	color(colors().text);
	draw::push();
		float sx = (dx- 1.f*mPad)/Glyph::width();
		float sy = (h - 2.f*mPad)/Glyph::baseline();
		scale(sx, sy);
		sx = 1.f/sx;

		float tdx = Glyph::width()*0.5f;
		float x = (w - dx*0.5f)*sx - tdx;
		float y = mPad/sy;
		dx *= sx;
		
		int power = 1;
		bool drawChar = false; // don't draw until non-zero or past decimal point

		// Draw the digits
		if(mNF  > 0) character('.', dx*(mNI+sizeSign()) - tdx, y);
		if(mShowSign && mVal < 0) character('-', dx*0.5 - tdx, y);
		
		for(int i=0; i<=msd; ++i){
			char c = '0' + (absVal % (power*10))/power;
			power *= 10;
			if(c!='0' || i>=mNF) drawChar = true;
			if(drawChar) character(c, x, y);
			x -= dx;
		}
	draw::pop();
}


bool NumberDialer::onEvent(Event::t e, GLV& g){

	Keyboard& k = g.keyboard;
	Mouse& m    = g.mouse;

	switch(e){
	case Event::MouseDown:{
		mAcc = 0;
		int oldDig = dig();
		dig((int)(m.xRel()/w * size()));
		if(dig() == 0 && oldDig == 0 && mShowSign) flipSign();
		return false;
	}
	
	case Event::MouseDrag:
		if(onNumber()){
			mAcc += 0.25 * fabs(m.dy());
			if(mAcc > 1){
				int mul = (int)mAcc;
				valAdd((m.dy() > 0.f ? -mag() : mag())*mul);
				mAcc -= mul;
			}
		}
		return false;
	
	case Event::KeyDown:
	
		if(k.isNumber() && onNumber()){
			int v = mVal < 0 ? -mVal : mVal;
			int p = mag();
			v += (k.keyAsNumber() - ((v / p) % 10)) * p;
			valSet(mVal < 0 ? -v : v);
			return false;
		}
		else{
			switch(k.key()){
			case 'a': onNumber() ? valAdd( mag()) : flipSign(); return false;
			case 'z': onNumber() ? valAdd(-mag()) : flipSign(); return false;
			case '-': flipSign(); return false;
			case 'c': value(0); return false;
			case '.': dig(size()-mNF); return false; // go to first fraction digit (if any)
			case Key::Right: dig(dig()+1); return false;
			case Key::Left:  dig(dig()-1); return false;
			}
		}
		break;
		
	default:;
	}

	return true;
}




TextView::TextView(const Rect& r)
:	View(r), mSpacing(1), mPadX(4), mSel(0), mBlink(0)
{
	setPos(0);
	size(8);
}

void TextView::callNotify(){ notify(Update::Value, TextViewChange(&mText)); }

TextView& TextView::size(float pixels){ mSize = pixels/draw::Glyph::width(); return *this; }

TextView& TextView::value(const std::string& v){
	if(v != mText){
		mText=v;
		callNotify();
	}
	return *this;
}

void TextView::onDraw(){
	using namespace draw;
	
	if(++mBlink==40) mBlink=0; // update blink interval
	
	float padX = mPadX/mSize;
	float padY = 4/mSize;
	float addY =-2/mSize;	// subtraction from top since some letters go above cap
	
	draw::push();
	draw::scale(mSize, mSize);

	float tl = mPos * Glyph::width() + padX;
	float tr = tl + Glyph::width();
	float tt = 0 * (mSpacing * Glyph::baseline()) + addY + padY;
	float tb = tt + Glyph::descent() - addY;
	
	// draw selection
	if(selected()){
		float sl, sr;
		if(mSel>0){
			sl = tl;
			sr = sl + mSel*Glyph::width();
		}
		else{
			sr = tl;
			sl = sr + mSel*Glyph::width();
		}
		color(colors().fore, colors().fore.a*0.4);
		rect(sl, tt, sr, tb);
	}

	// draw cursor
	if(mBlink<20 && enabled(Focused)){
		color(colors().text);
		shape(Lines, tl, tt, tl, tb);
	}

	color(colors().text);
	draw::text(mText.c_str(), padX, padY);
	draw::pop();
}

bool TextView::onEvent(Event::t e, GLV& g){

	const Keyboard& k = g.keyboard;
	int key = k.key();
	float mx = g.mouse.xRel();

	switch(e){
		case Event::KeyDown:
			if(isprint(key)){
				deleteSelected();
				mText.insert(mPos, 1, k.key()); callNotify();
				setPos(mPos+1);
				return false;
			}
			else{
				switch(key){
				case Key::Delete:
					if(selected()) deleteSelected();
					else if(validPos()){
						deleteText(mPos-1, 1);
						setPos(mPos-1);
					}
					return false;
					
				case Key::BackSpace:
					if(selected()) deleteSelected();
					else if(mText.size()){
						deleteText(mPos, 1);
						setPos(mPos);
					}
					return false;
				
				case Key::Left:
					if(k.shift()) select(mSel-1);
					else setPos(mPos-1);
					return false;
					
				case Key::Right:
					if(k.shift()) select(mSel+1);
					else setPos(mPos+1);
					return false;
					
				case Key::Down:	setPos(mText.size()); return false;
				case Key::Up:	setPos(0); return false;
					
				case Key::Enter:
				case Key::Return:
					return false;
				}
			}
			break;

		case Event::MouseDown:
			setPos(xToPos(mx));
			break;

		case Event::MouseDrag:
			{
				int p = xToPos(mx);
				if(p >= mPos) select(p-mPos+1);
				else select(p-mPos);
				//printf("%d\n", mSel);
			}
			break;

		default:;
	}

	return true;
}


void TextView::deleteSelected(){
	if(mSel>0){
		deleteText(mPos, mSel);
		setPos(mPos);
	}
	else if(mSel<0){
		deleteText(mPos+mSel, mPos);
		setPos(mPos+mSel);
	}
}


void TextView::deleteText(int start, int num){
	mText.erase(start, num);
	callNotify();
}

void TextView::select(int v){
	int nt = mText.size();
	int end = mPos + v;
	if(end<0) end=0;
	if(end>nt) end = nt;
	mSel = end-mPos;
}

void TextView::setPos(int v){
	if(v<=int(mText.size()) && v>=0){
		mPos=v;
	}
	deselect();
	mBlink=0;
}

int TextView::xToPos(float x){
	float charw = draw::Glyph::width() * mSize;
	if(x<0) x=0;
	int p = (x-mPadX*1)/charw;
	if(p > (int)mText.size()) p = mText.size();
	if(p<0) p=0;
	return p;
}




} // glv::