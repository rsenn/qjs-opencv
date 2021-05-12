#include "color.hpp"

/*
 * H(Hue): 0 - 360 degree (integer)
 * S(Saturation): 0 - 1.00 (double)
 * V(Value): 0 - 1.00 (double)
 *
 * output[3]: Output, std::array size 3, int
 */
color_type
hsv_to_rgb(int H, double S, double V) {
  double C = S * V;
  double X = C * (1 - abs(fmod(H / 60.0, 2) - 1));
  double m = V - C;
  double Rs, Gs, Bs;

  if(H >= 0 && H < 60) {
    Rs = C;
    Gs = X;
    Bs = 0;
  } else if(H >= 60 && H < 120) {
    Rs = X;
    Gs = C;
    Bs = 0;
  } else if(H >= 120 && H < 180) {
    Rs = 0;
    Gs = C;
    Bs = X;
  } else if(H >= 180 && H < 240) {
    Rs = 0;
    Gs = X;
    Bs = C;
  } else if(H >= 240 && H < 300) {
    Rs = X;
    Gs = 0;
    Bs = C;
  } else {
    Rs = C;
    Gs = 0;
    Bs = X;
  }

  return color_type((int)((Rs + m) * 255), (int)((Gs + m) * 255), (int)((Bs + m) * 255));
}
