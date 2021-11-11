import * as cv from 'opencv';

function main(...args) {
  let images = [];
  let palette = [],
    palette2 = [],
    palette3 = [];
  let size = new cv.Size(640, 480);

  let paletteImage = cv.imread('lsd/images/building.jpg');

  palette = cv.paletteGenerate(paletteImage, (0 << 1) + 1, 16);

  for(let i = 0; i < 10; i++) {
    let mat = new cv.Mat(size, cv.CV_8UC4);
    // paletteImage.copyTo(mat);

    images.push(mat);
  }
  /*
  for(let i = 0; i < 15; i++) {
    palette.push([i & 1, i & 2, i & 4].map(n => (n ? (i & 8 ? 255 : 128) : 0)));
  }
  palette.unshift([255, 170, 85]);*/

  const ansiColor = (r, g, b, bg = false) => `\x1b[${bg ? 48 : 38};2;${r};${g};${b}m`;
  const noColor = () => `\x1b[0m`;
  const colorize = (text, color) => ansiColor(...color, true) + text + noColor();

  console.log(images);
  for(let i = 0; i < palette.length; i++) {
    const [r, g, b] = palette[i];

    console.log(ansiColor(r, g, b, true) + `palette #${i}` + noColor(), palette[i]);
  }
  for(let i = 0; i < 16; i++) {
    palette2[i] = [i & 1, i & 2, i & 4].map(n => !!n | 0).map(n => n * (i & 8 ? 255 : 128));
  }
  const hsv_pal = new cv.Mat(1, 16, cv.CV_8UC3),
    bgr_pal = new cv.Mat(1, 16, cv.CV_8UC3);
  {
    let i = 0;
    for(let color of hsv_pal) {
      palette3[i] = [0, 0, 0];
      color[0] = ((i % 8) * 179) / 8;
      color[1] = i & 8 ? 192 : 255;
      color[2] = i & 8 ? 255 : 100;
      i++;
    }
  }

  const toHex = n => n.toString(16).padStart(2, '0');
  const toDec = n => n.toString(10).padStart(3, ' ');
  const hexColor = (...args) => '#' + args.map(toHex).join('');
  const decTriplet = (...args) => args.map(toDec).join(', ');

  const randInt = max => Math.floor(Math.random() * max);
  const randCoord = () => new cv.Point(randInt(size.width), randInt(size.height));

  const dumpPalette = (palette, fmt = hexColor) =>
    [...palette].map((c, i) => i + ' ' + colorize(fmt(...c), c)).join(', ');
  console.log('hsv_pal', hsv_pal);
  console.log('hsv_pal', dumpPalette(hsv_pal, decTriplet));

  cv.cvtColor(hsv_pal, bgr_pal, cv.COLOR_HSV2BGR);
  console.log('bgr_pal', bgr_pal);
  palette3 = [...bgr_pal];
  console.log('palette3', dumpPalette(palette3));

  for(let i = 0; i < images.length; i++) {
    for(let j = 0; j < 15; j++) {
      let coords = [randCoord(), randCoord()];
      let color = palette3[j];
      console.log('coords', coords.map(({ x, y }) => x + ',' + y).join(' -> '));
      console.log('color', color);
      console.log('j', j);
      cv.line(images[i], ...coords, [...color.slice(0, 3), 255], 3, false);
    }
    for(let j = 0; j < 15; j++) {
      let color = palette3[j];
      let rect = new cv.Rect(new cv.Point(0, j * 20), new cv.Point(100, (j + 1) * 20));
      cv.rectangle(images[i], rect, [...color.slice(0, 3), 255], cv.FILLED, false);
    }

    cv.imwrite(`image-${i}.png`, images[i]);
  }
  /*
  let mats = images.map(im => {
    let mat = new cv.Mat(im.size, cv.CV_8UC1);

    cv.cvtColor(im, mat, cv.COLOR_BGR2GRAY);
    return mat;
  });*/
  cv.imwrite('output.gif', images, palette3, 100, 15, 0);
}

main(...scriptArgs.slice(1));
