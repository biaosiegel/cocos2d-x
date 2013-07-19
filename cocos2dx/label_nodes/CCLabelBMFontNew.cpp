/****************************************************************************
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2008-2010 Ricardo Quesada
Copyright (c) 2011      Zynga Inc.

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

Use any of these editors to generate BMFonts:
http://glyphdesigner.71squared.com/ (Commercial, Mac OS X)
http://www.n4te.com/hiero/hiero.jnlp (Free, Java)
http://slick.cokeandcode.com/demos/hiero.jnlp (Free, Java)
http://www.angelcode.com/products/bmfont/ (Free, Windows only)

****************************************************************************/
#include "CCLabelBMFontNew.h"
#include "cocoa/CCString.h"
#include "platform/platform.h"
#include "cocoa/CCDictionary.h"
#include "CCConfiguration.h"
#include "CCLabelTextFormatter.h"
#include "draw_nodes/CCDrawingPrimitives.h"
#include "sprite_nodes/CCSprite.h"
//#include "support/CCPointExtension.h"
#include "platform/CCFileUtils.h"
#include "CCDirector.h"
#include "textures/CCTextureCache.h"
#include "support/ccUTF8.h"

using namespace std;

NS_CC_BEGIN


// The return value needs to be deleted by CC_SAFE_DELETE_ARRAY.
static unsigned short* copyUTF16StringNN(unsigned short* str)
{
    int length = str ? cc_wcslen(str) : 0;
    unsigned short* ret = new unsigned short[length+1];
    for (int i = 0; i < length; ++i) {
        ret[i] = str[i];
    }
    ret[length] = 0;
    return ret;
}

//
//CCLabelBMFont
//

//LabelBMFont - Purge Cache
void LabelBMFontNew::purgeCachedData()
{
    FNTConfigRemoveCache();
}

LabelBMFontNew * LabelBMFontNew::create()
{
    LabelBMFontNew * pRet = new LabelBMFontNew();
    if (pRet && pRet->init())
    {
        pRet->autorelease();
        return pRet;
    }
    CC_SAFE_DELETE(pRet);
    return NULL;
}

LabelBMFontNew * LabelBMFontNew::create(const char *str, const char *fntFile, float width, TextAlignment alignment)
{
    return LabelBMFontNew::create(str, fntFile, width, alignment, Point::ZERO);
}

LabelBMFontNew * LabelBMFontNew::create(const char *str, const char *fntFile, float width)
{
    return LabelBMFontNew::create(str, fntFile, width, kTextAlignmentLeft, Point::ZERO);
}

LabelBMFontNew * LabelBMFontNew::create(const char *str, const char *fntFile)
{
    return LabelBMFontNew::create(str, fntFile, kLabelAutomaticWidth, kTextAlignmentLeft, Point::ZERO);
}

//LabelBMFont - Creation & Init
LabelBMFontNew *LabelBMFontNew::create(const char *str, const char *fntFile, float width/* = kLabelAutomaticWidth*/, TextAlignment alignment/* = kTextAlignmentLeft*/, Point imageOffset/* = Point::ZERO*/)
{
    LabelBMFontNew *pRet = new LabelBMFontNew();
    if(pRet && pRet->initWithString(str, fntFile, width, alignment, imageOffset))
    {
        pRet->autorelease();
        return pRet;
    }
    CC_SAFE_DELETE(pRet);
    return NULL;
}

bool LabelBMFontNew::init()
{
    return initWithString(NULL, NULL, kLabelAutomaticWidth, kTextAlignmentLeft, Point::ZERO);
}

bool LabelBMFontNew::initWithString(const char *theString, const char *fntFile, float width/* = kLabelAutomaticWidth*/, TextAlignment alignment/* = kTextAlignmentLeft*/, Point imageOffset/* = Point::ZERO*/)
{
    CCAssert(!_configuration, "re-init is no longer supported");
    CCAssert( (theString && fntFile) || (theString==NULL && fntFile==NULL), "Invalid params for LabelBMFontNew");
    
    Texture2D *texture = NULL;
    
    if (fntFile)
    {
        CCBMFontConfiguration *newConf = FNTConfigLoadFile(fntFile);
        if (!newConf)
        {
            CCLOG("cocos2d: WARNING. LabelBMFontNew: Impossible to create font. Please check file: '%s'", fntFile);
            release();
            return false;
        }
        
        newConf->retain();
        CC_SAFE_RELEASE(_configuration);
        _configuration = newConf;
        
        _fntFile = fntFile;
        
        texture = TextureCache::getInstance()->addImage(_configuration->getAtlasName());
    }
    else 
    {
        texture = new Texture2D();
        texture->autorelease();
    }

    if (theString == NULL)
    {
        theString = "";
    }

    if (SpriteBatchNode::initWithTexture(texture, strlen(theString)))
    {
        _width = width;
        _alignment = alignment;
        
        _displayedOpacity = _realOpacity = 255;
		_displayedColor = _realColor = Color3B::WHITE;
        _cascadeOpacityEnabled = true;
        _cascadeColorEnabled = true;
        
        _contentSize = Size::ZERO;
        
        _isOpacityModifyRGB = _textureAtlas->getTexture()->hasPremultipliedAlpha();
        _anchorPoint = Point(0.5f, 0.5f);
        
        _imageOffset = imageOffset;
        
        _reusedChar = new Sprite();
        _reusedChar->initWithTexture(_textureAtlas->getTexture(), Rect(0, 0, 0, 0), false);
        _reusedChar->setBatchNode(this);
        
        this->setString(theString);
        
        return true;
    }
    return false;
}

LabelBMFontNew::LabelBMFontNew()
: _string(NULL)
, _initialString(NULL)
, _alignment(kTextAlignmentCenter)
, _width(-1.0f)
, _configuration(NULL)
, _lineBreakWithoutSpaces(false)
, _imageOffset(Point::ZERO)
, _reusedChar(NULL)
, _displayedOpacity(255)
, _realOpacity(255)
, _displayedColor(Color3B::WHITE)
, _realColor(Color3B::WHITE)
, _cascadeColorEnabled(true)
, _cascadeOpacityEnabled(true)
, _isOpacityModifyRGB(false)
{

}

LabelBMFontNew::~LabelBMFontNew()
{
    CC_SAFE_RELEASE(_reusedChar);
    CC_SAFE_DELETE_ARRAY(_string);
    CC_SAFE_DELETE_ARRAY(_initialString);
    CC_SAFE_RELEASE(_configuration);
}

// LabelBMFontNew - Atlas generation
int LabelBMFontNew::kerningAmountForFirst(unsigned short first, unsigned short second)
{
    int ret = 0;
    unsigned int key = (first<<16) | (second & 0xffff);

    if( _configuration->_kerningDictionary ) {
        tKerningHashElement *element = NULL;
        HASH_FIND_INT(_configuration->_kerningDictionary, &key, element);        
        if(element)
            ret = element->amount;
    }
    return ret;
}

const char* LabelBMFontNew::getString(void) const
{
    return _initialStringUTF8.c_str();
}

//LabelBMFontNew - RGBAProtocol protocol
const Color3B& LabelBMFontNew::getColor() const
{
    return _realColor;
}

const Color3B& LabelBMFontNew::getDisplayedColor() const
{
    return _displayedColor;
}

void LabelBMFontNew::setColor(const Color3B& color)
{
	_displayedColor = _realColor = color;
	
	if( _cascadeColorEnabled ) {
		Color3B parentColor = Color3B::WHITE;
        RGBAProtocol* pParent = dynamic_cast<RGBAProtocol*>(_parent);
        if (pParent && pParent->isCascadeColorEnabled())
        {
            parentColor = pParent->getDisplayedColor();
        }
        this->updateDisplayedColor(parentColor);
	}
}

GLubyte LabelBMFontNew::getOpacity(void) const
{
    return _realOpacity;
}

GLubyte LabelBMFontNew::getDisplayedOpacity(void) const
{
    return _displayedOpacity;
}

/** Override synthesized setOpacity to recurse items */
void LabelBMFontNew::setOpacity(GLubyte opacity)
{
	_displayedOpacity = _realOpacity = opacity;
    
	if( _cascadeOpacityEnabled ) {
		GLubyte parentOpacity = 255;
        RGBAProtocol* pParent = dynamic_cast<RGBAProtocol*>(_parent);
        if (pParent && pParent->isCascadeOpacityEnabled())
        {
            parentOpacity = pParent->getDisplayedOpacity();
        }
        this->updateDisplayedOpacity(parentOpacity);
	}
}

void LabelBMFontNew::setOpacityModifyRGB(bool var)
{
    _isOpacityModifyRGB = var;
    if (_children && _children->count() != 0)
    {
        Object* child;
        CCARRAY_FOREACH(_children, child)
        {
            Node* pNode = static_cast<Node*>( child );
            if (pNode)
            {
                RGBAProtocol *pRGBAProtocol = dynamic_cast<RGBAProtocol*>(pNode);
                if (pRGBAProtocol)
                {
                    pRGBAProtocol->setOpacityModifyRGB(_isOpacityModifyRGB);
                }
            }
        }
    }
}
bool LabelBMFontNew::isOpacityModifyRGB() const
{
    return _isOpacityModifyRGB;
}

void LabelBMFontNew::updateDisplayedOpacity(GLubyte parentOpacity)
{
	_displayedOpacity = _realOpacity * parentOpacity/255.0;
    
	Object* pObj;
	CCARRAY_FOREACH(_children, pObj)
    {
        Sprite *item = static_cast<Sprite*>( pObj );
		item->updateDisplayedOpacity(_displayedOpacity);
	}
}

void LabelBMFontNew::updateDisplayedColor(const Color3B& parentColor)
{
	_displayedColor.r = _realColor.r * parentColor.r/255.0;
	_displayedColor.g = _realColor.g * parentColor.g/255.0;
	_displayedColor.b = _realColor.b * parentColor.b/255.0;
    
    Object* pObj;
	CCARRAY_FOREACH(_children, pObj)
    {
        Sprite *item = static_cast<Sprite*>( pObj );
		item->updateDisplayedColor(_displayedColor);
	}
}

bool LabelBMFontNew::isCascadeColorEnabled() const
{
    return false;
}

void LabelBMFontNew::setCascadeColorEnabled(bool cascadeColorEnabled)
{
    _cascadeColorEnabled = cascadeColorEnabled;
}

bool LabelBMFontNew::isCascadeOpacityEnabled() const
{
    return false;
}

void LabelBMFontNew::setCascadeOpacityEnabled(bool cascadeOpacityEnabled)
{
    _cascadeOpacityEnabled = cascadeOpacityEnabled;
}

// LabelBMFontNew - AnchorPoint
void LabelBMFontNew::setAnchorPoint(const Point& point)
{
    if( ! point.equals(_anchorPoint))
    {
        SpriteBatchNode::setAnchorPoint(point);
        updateLabel();
    }
}

// LabelBMFontNew - Alignment
void LabelBMFontNew::setAlignment(TextAlignment alignment)
{
    this->_alignment = alignment;
    updateLabel();
}

void LabelBMFontNew::setWidth(float width)
{
    this->_width = width;
    updateLabel();
}

void LabelBMFontNew::setLineBreakWithoutSpace( bool breakWithoutSpace )
{
    _lineBreakWithoutSpaces = breakWithoutSpace;
    updateLabel();
}

void LabelBMFontNew::setScale(float scale)
{
    SpriteBatchNode::setScale(scale);
    updateLabel();
}

void LabelBMFontNew::setScaleX(float scaleX)
{
    SpriteBatchNode::setScaleX(scaleX);
    updateLabel();
}

void LabelBMFontNew::setScaleY(float scaleY)
{
    SpriteBatchNode::setScaleY(scaleY);
    updateLabel();
}

float LabelBMFontNew::getLetterPosXLeft( Sprite* sp )
{
    return sp->getPosition().x * _scaleX - (sp->getContentSize().width * _scaleX * sp->getAnchorPoint().x);
}

float LabelBMFontNew::getLetterPosXRight( Sprite* sp )
{
    return sp->getPosition().x * _scaleX + (sp->getContentSize().width * _scaleX * sp->getAnchorPoint().x);
}

// LabelBMFontNew - FntFile
void LabelBMFontNew::setFntFile(const char* fntFile)
{
    if (fntFile != NULL && strcmp(fntFile, _fntFile.c_str()) != 0 )
    {
        CCBMFontConfiguration *newConf = FNTConfigLoadFile(fntFile);

        CCAssert( newConf, "CCLabelBMFontNew: Impossible to create font. Please check file");

        _fntFile = fntFile;

        CC_SAFE_RETAIN(newConf);
        CC_SAFE_RELEASE(_configuration);
        _configuration = newConf;

        this->setTexture(TextureCache::getInstance()->addImage(_configuration->getAtlasName()));
        LabelTextFormatter::createStringSprites(this);
    }
}

const char* LabelBMFontNew::getFntFile()
{
    return _fntFile.c_str();
}


//LabelBMFontNew - Debug draw
#if CC_LabelBMFontNew_DEBUG_DRAW
void LabelBMFontNew::draw()
{
    SpriteBatchNode::draw();
    const Size& s = this->getContentSize();
    Point vertices[4]={
        ccp(0,0),ccp(s.width,0),
        ccp(s.width,s.height),ccp(0,s.height),
    };
    ccDrawPoly(vertices, 4, true);
}

#endif // CC_LABELBMFONT_DEBUG_DRAW


int  LabelBMFontNew::getCommonLineHeight()
{
    if (_configuration)
    {
        return _configuration->_commonHeight;
    }
    else
    {
        return -1;
    }
}

int  LabelBMFontNew::getKerningForCharsPair(unsigned short first, unsigned short second)
{
    return this->kerningAmountForFirst(first, second);
}

ccBMFontDef LabelBMFontNew::getFontDefForChar(unsigned short int theChar)
{
    ccBMFontDef fontDef;
    tFontDefHashElement *element = NULL;
    unsigned int key = theChar;
    
    HASH_FIND_INT(_configuration->_fontDefDictionary, &key, element);
    
    if (element)
    {
        fontDef = element->fontDef;
    }
    
    return fontDef;
}

// return a sprite for rendering one letter
Sprite * LabelBMFontNew::getSpriteForChar(unsigned short int theChar, int spriteIndexHint)
{
    Rect      rect;
    ccBMFontDef fontDef;
    Sprite   *pRetSprite = 0;
    
    // unichar is a short, and an int is needed on HASH_FIND_INT
    tFontDefHashElement *element  = NULL;
    unsigned int key              = theChar;
    
    HASH_FIND_INT(_configuration->_fontDefDictionary, &key, element);
    
    if (! element)
    {
        return 0;
    }
    
    fontDef = element->fontDef;
    
    rect = fontDef.rect;
    rect = CC_RECT_PIXELS_TO_POINTS(rect);
    
    rect.origin.x += _imageOffset.x;
    rect.origin.y += _imageOffset.y;
    
    //bool hasSprite  = true;
    pRetSprite      = (Sprite*)(this->getChildByTag(spriteIndexHint));
    
    if(pRetSprite )
    {
        // Reusing previous Sprite
        pRetSprite->setVisible(true);
    }
    else
    {
        pRetSprite = new Sprite();
        pRetSprite->initWithTexture(_textureAtlas->getTexture(), rect);
        addChild(pRetSprite, spriteIndexHint, spriteIndexHint);
        pRetSprite->release();
        
        // Apply label properties
        pRetSprite->setOpacityModifyRGB(_isOpacityModifyRGB);
        
        // Color MUST be set before opacity, since opacity might change color if OpacityModifyRGB is on
        pRetSprite->updateDisplayedColor(_displayedColor);
        pRetSprite->updateDisplayedOpacity(_displayedOpacity);
    }
    
    // updating previous sprite
    pRetSprite->setTextureRect(rect, false, rect.size);
    return pRetSprite;
}

int LabelBMFontNew::getStringNumLines()
{
    int quantityOfLines = 1;
    
    unsigned int stringLen = _string ? cc_wcslen(_string) : 0;
    if (stringLen == 0)
        return (-1);
    
    // count number of lines
    for (unsigned int i = 0; i < stringLen - 1; ++i)
    {
        unsigned short c = _string[i];
        if (c == '\n')
        {
            quantityOfLines++;
        }
    }
    
    return quantityOfLines;
}

// need cross implementation
int LabelBMFontNew::getStringLenght()
{
    return _string ? cc_wcslen(_string) : 0;
}

unsigned short LabelBMFontNew::getCharAtStringPosition(int position)
{
    return _string[position];
}

int LabelBMFontNew::getXOffsetForChar(unsigned short c)
{
    ccBMFontDef fontDef = getFontDefForChar(c);
    return fontDef.xOffset;
}

int LabelBMFontNew::getYOffsetForChar(unsigned short c)
{
    ccBMFontDef fontDef = getFontDefForChar(c);
    return fontDef.yOffset;
}

Rect LabelBMFontNew::getRectForChar(unsigned short c)
{
    ccBMFontDef fontDef = getFontDefForChar(c);
    return fontDef.rect;
}

int LabelBMFontNew::getAdvanceForChar(unsigned short c, int hintPositionInString)
{
    ccBMFontDef fontDef = getFontDefForChar(c);
    return fontDef.xAdvance;
}

void  LabelBMFontNew::setLabelContentSize(const Size &newSize)
{
    setContentSize(newSize);
}

void LabelBMFontNew::createStringSprites()
{
    LabelTextFormatter::createStringSprites(this);
}

void LabelBMFontNew::setString(const char *newString)
{
    // store initial string in char8 format
     _initialStringUTF8 = newString;
    
    // update the initial string if needed
    unsigned short* utf16String = cc_utf8_to_utf16(newString);
    unsigned short* tmp         = _initialString;
    _initialString              = copyUTF16StringNN(utf16String);

    CC_SAFE_DELETE_ARRAY(tmp);
    CC_SAFE_DELETE_ARRAY(utf16String);
    
    // do the rest of the josb
    updateLabel();
}

void LabelBMFontNew::setCString(const char *label)
{
    setString(label);
}

void LabelBMFontNew::updateLabel()
{
    if ( _initialString!=0 )
    {
        // set the new string
        CC_SAFE_DELETE_ARRAY(_string);
        _string = copyUTF16StringNN(_initialString);
        
        // hide all the letters and create or recicle sprites for the new letters
        updateLetterSprites();
        
        // format the text on more than line
        multilineText();
        
        // align the text (left - center - right)
        alignText();
    }
}

void LabelBMFontNew::updateLetterSprites()
{
    // hide all the letters
    hideStringSprites();
    
    // create new letters sprites
    createStringSprites();
}

void LabelBMFontNew::hideStringSprites()
{
    if (_children && _children->count() != 0)
    {
        Object* child;
        CCARRAY_FOREACH(_children, child)
        {
            Node* pNode = (Node*) child;
            if (pNode)
            {
                pNode->setVisible(false);
            }
        }
    }
}

void LabelBMFontNew::multilineText()
{
    if (_width > 0)
    {
        // format on more than one line
        LabelTextFormatter::multilineText(this);
        
        // hide all the letter sprites and create/reclaim letters sprite with new position
        updateLetterSprites();
    }
}

void LabelBMFontNew::alignText()
{
    if (_alignment != kTextAlignmentLeft)
    {
        LabelTextFormatter::alignText(this);
    }
}

unsigned short * LabelBMFontNew::getUTF8String()
{
    return _string;
}

Sprite * LabelBMFontNew::getSpriteChild(int ID)
{
    return (Sprite*)this->getChildByTag(ID);
}

float LabelBMFontNew::getMaxLineWidth()
{
    return _width;
}

TextAlignment LabelBMFontNew::getTextAlignment()
{
    return _alignment;
}

Array*  LabelBMFontNew::getChildrenLetters()
{
    return _children;
}

void LabelBMFontNew::assignNewUTF8String(unsigned short *newString)
{
    CC_SAFE_DELETE_ARRAY(_string);
    _string = newString;
}

Size LabelBMFontNew::getLabelContentSize()
{
    return getContentSize();
}

bool LabelBMFontNew::breakLineWithoutSpace()
{
    return _lineBreakWithoutSpaces;
}


NS_CC_END
