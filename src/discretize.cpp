/***************************************************************************
                          discretize.cpp  -  description
                             -------------------
    begin                : Sun Dec 22 2003
    copyright            : (C) 2003 by
    email                : mmdigitizer@earthlink.net
    $Log: discretize.cpp,v $
    Revision 1.6  2008/05/04 16:14:41  tsrsf
    update to reflect lastest Debian package
    change outside /debian/ allow compiling without qt 3 compat headers

    Revision 1.5  2006/10/08 06:41:49  markmitch
    NO_UNDO branch moved to HEAD

    Revision 1.3  2005/03/20 01:47:05  markmitch
    After KDevelop 3 restructuring

    Revision 1.17  2004/11/24 06:00:53  markmitch
    Remove compiler warning

    Revision 1.16  2004/11/23 05:30:06  markmitch
    Changes so segment preview works in OSX

    Revision 1.15  2004/09/27 04:52:26  markmitch
    KDevelop does not allow renaming source directory to src

    Revision 1.13  2004/09/12 22:29:21  markmitch
    Settings archival complete. Qt2-compatibility code removed

    Revision 1.12  2004/01/28 23:28:41  markmitch
    Rewrite of one-bit images

    Revision 1.11  2004/01/20 05:48:53  markmitch
    Instantaneous response in dialogs

    Revision 1.10  2004/01/08 06:48:35  markmitch
    Reworked menu icons look great

    Revision 1.9  2004/01/06 08:14:39  markmitch
    Point match mode highlights sample point

    Revision 1.8  2004/01/04 08:40:59  markmitch
    Dialog for point match settings

    Revision 1.7  2003/12/30 09:14:32  markmitch
    Color chooser has two formats and can be disabled

    Revision 1.6  2003/12/29 20:51:50  markmitch
    All discretize modes work

    Revision 1.5  2003/12/29 06:55:10  markmitch
    Cvs log cleanup. Color chooser


 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <math.h>
#include <qapplication.h>
//Added by qt3to4:
#include <QList>

#include "main.h"
#include "mmsubs.h"
#include "digitdebug.h"
#include "discretize.h"

Discretize::Discretize()
{
  DigitDebug::ctor(QString("discretize ") + QString::number((ulong) this, 16));
}

Discretize::~Discretize()
{
  DigitDebug::dtor(QString("discretize ") + QString::number((ulong) this, 16));
}

QRgb Discretize::backgroundColor(const QImage* image)
{
  // travel diagonally (top-left to bottom-right) and count colors to isolate
  // background color, which is assumed to be the most frequent color

  QList<ColorEntry>::iterator itrC;

  // add unique elements to colors list
  ColorList colors;
  int x, y = 0;
  for (x = 0; x < image->width(); x++)
  {
    // look up this new pixel color
    ColorEntry c;
    c.color = pixelRGB(image, x, y);
    c.count = 0;

    bool found = false;
    for (itrC = colors.begin(); itrC != colors.end(); ++itrC)
    {
      if (colorCompare(c.color.rgb(), (*itrC).color.rgb()))
      {
        found = true;
        ++(*itrC).count;
        break;
      }
    }
    if (!found)
      colors.append(c);

    if (++y >= image->height())
      y = 0;
  }

  // background color is most frequent color
  ColorEntry cMax;
  cMax.count = 0;
  for (itrC = colors.begin(); itrC != colors.end(); ++itrC)
  {
    if ((*itrC).count > cMax.count)
      cMax = (*itrC);
  }

  return cMax.color.rgb();
}

int Discretize::colorAttributeMax(DiscretizeMethod method) const
{
  switch (method)
  {
  case DiscretizeNone:
    return 0;
  case DiscretizeIntensity:
    return DiscretizeIntensityMax;
  case DiscretizeForeground:
    return DiscretizeForegroundMax;
  case DiscretizeHue:
    return DiscretizeHueMax;
  case DiscretizeSaturation:
    return DiscretizeSaturationMax;
  case DiscretizeValue:
  default:
    return DiscretizeValueMax;
  }
}

bool Discretize::colorCompare(QRgb color1, QRgb color2)
{
  const long MASK = 0xf0f0f0f0;
  return (color1 & MASK) == (color2 & MASK);
}

void Discretize::discretize(QImage* image, DiscretizeSettings settings, QRgb backgroundColor)
{
  int x, y, rBg, gBg, bBg;
  QColor background(backgroundColor);
  background.rgb(&rBg, &gBg, &bBg);

  // save the most recent settings
  static DiscretizeSettings settingsLatest;
  settingsLatest = settings;

  int value;
  for (x = 0; x < image->width(); x++)
  {
    for (y = 0; y < image->height(); y++)
    {
      if (settings.discretizeMethod == DiscretizeForeground)
        value = discretizeValueForeground(image, x, y, settings.discretizeMethod, rBg, gBg, bBg);
      else
        value = discretizeValueNotForeground(image, x, y, settings.discretizeMethod);

      if (pixelIsOn(value, settings))
        setPixelRGB(image, x, y, QColor(Qt::black).rgb());
      else
        setPixelRGB(image, x, y,  QColor(Qt::white).rgb());
    }

    // abort all calls to this function except the most recent, which last defined settingsLatest
    qApp->processEvents();
    if (memcmp(&settings, &settingsLatest, sizeof(settingsLatest)) != 0)
      break;
  }
}

int Discretize::discretizeValueForeground(const QImage* image, int x, int y, DiscretizeMethod method,
  int rBg, int gBg, int bBg) const
{
  // convert foreground value
  ASSERT_ENGAUGE(method == DiscretizeForeground);

  QColor color(pixelRGB(image, x, y));

  int r, g, b;
  double distance;
  color.rgb(&r, &g, &b);
  distance = sqrt ((r - rBg) * (r - rBg) + (g - gBg) * (g - gBg) + (b - bBg) * (b - bBg));
  int value = (int) (distance * DiscretizeForegroundMax / sqrt(255 * 255 + 255 * 255 + 255 * 255) + 0.5);

  if (value < 0)
    value = 0;
  if (colorAttributeMax(method) < value)
    value = colorAttributeMax(method);

  return value;
}

int Discretize::discretizeValueNotForeground(const QImage* image, int x, int y, DiscretizeMethod method) const
{
  // convert intensity, hue, threshold or saturation value
  ASSERT_ENGAUGE(method != DiscretizeForeground);
  
  QColor color(pixelRGB(image, x, y));

  int h, s, v, r, g, b;
  double intensity;

  // convert hue from 0 to 359, saturation from 0 to 255, value from 0 to 255
  int value = 0;
  switch (method)
  {
  case DiscretizeNone:
    ASSERT_ENGAUGE(0);
    break;
  case DiscretizeIntensity:
    color.rgb(&r, &g, &b);
    intensity = sqrt (r * r + g * g + b * b);
    value = (int) (intensity * DiscretizeIntensityMax / sqrt(255 * 255 + 255 * 255 + 255 * 255) + 0.5);
    break;
  case DiscretizeForeground:
    break;
  case DiscretizeHue:
    color.hsv(&h, &s, &v);
    value = h * DiscretizeHueMax / 359;
    break;
  case DiscretizeSaturation:
    color.hsv(&h, &s, &v);
    value = s * DiscretizeSaturationMax / 255;
    break;
  case DiscretizeValue:
    color.hsv(&h, &s, &v);
    value = v * DiscretizeValueMax / 255;
    break;
  }

  if (value < 0)
    value = 0;
  if (colorAttributeMax(method) < value)
    value = colorAttributeMax(method);

  return value;
}

bool Discretize::pixelIsOn(int value, int low, int high) const
{
  if (low < high)
    return ((low <= value) && (value <= high));
  else
    return ((low <= value) || (value <= high));
}

bool Discretize::pixelIsOn(int value, GridRemovalSettings settings) const
{
  return pixelIsOn(value, settings.foregroundThresholdLow, settings.foregroundThresholdHigh);
}

bool Discretize::pixelIsOn(int value, DiscretizeSettings settings) const
{
  switch (settings.discretizeMethod)
  {
  case DiscretizeNone:
    ASSERT_ENGAUGE(0);
    break;
  case DiscretizeIntensity:
    return pixelIsOn(value, settings.intensityThresholdLow, settings.intensityThresholdHigh);
  case DiscretizeForeground:
    return pixelIsOn(value, settings.foregroundThresholdLow, settings.foregroundThresholdHigh);
  case DiscretizeHue:
    return pixelIsOn(value, settings.hueThresholdLow, settings.hueThresholdHigh);
  case DiscretizeSaturation:
    return pixelIsOn(value, settings.saturationThresholdLow, settings.saturationThresholdHigh);
  case DiscretizeValue:
    return pixelIsOn(value, settings.valueThresholdLow, settings.valueThresholdHigh);
  }

  return false;
}

bool Discretize::processedPixelIsOn(const QImage &image, int x, int y) const
{
  if ((x < 0) || (image.width() <= x) ||
    (y < 0) || (image.height() <= y))
  {
    return false;
  }

  // pixel is on if it is closer to black than white in gray scale. this test must be performed
  // on little endian and big endian systems, with or without alpha bits (which are typically high bits)
  const int BLACK_WHITE_THRESHOLD = 255 / 2; // put threshold in middle of range
  int gray = qGray(pixelRGB(&image, x, y));
  return (gray < BLACK_WHITE_THRESHOLD);
}
