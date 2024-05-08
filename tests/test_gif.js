import * as cv from 'opencv';

const NUM_IMAGES = 30;
const NUM_COLORS = 16;

function main(...args) {
  let images = [];
  let palette = [],
    palette2 = [],
    palette3 = [];
  let size = new cv.Size(640, 480);

  let paletteImage = cv.imread('lsd/images/building.jpg');

  palette = cv.paletteGenerate(paletteImage, (0 << 1) + 1, NUM_COLORS);

  for(let i = 0; i < NUM_IMAGES; i++) {
    let mat = new cv.Mat(size, cv.CV_8UC4);
    images.push(mat);
  }

  const ansiColor = (r, g, b, bg = false) => `\x1b[${bg ? 48 : 38};2;${r};${g};${b}m`;
  const noColor = () => `\x1b[0m`;
  const colorize = (text, color) => ansiColor(...color, true) + text + noColor();

  for(let i = 0; i < palette.length; i++) {
    const [r, g, b] = palette[i];

    console.log(ansiColor(r, g, b, true) + `palette #${i}` + noColor(), palette[i]);
  }

  for(let i = 0; i < NUM_COLORS; i++) {
    palette2[i] = [i & 1, i & 2, i & 4].map(n => !!n | 0).map(n => n * (i & 8 ? 255 : 128));
  }

  const hsv_pal = new cv.Mat(1, NUM_COLORS, cv.CV_8UC3),
    bgr_pal = new cv.Mat(1, NUM_COLORS, cv.CV_8UC3);
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
  const randCoord = () => new cv.Point(randInt(size.width - 100) + 100, randInt(size.height));
  const randLine = () => new cv.Line(...randCoord(), ...randCoord());

  const dumpPalette = (palette, fmt = hexColor) => [...palette].map((c, i) => i + ' ' + colorize(fmt(...c), c)).join(', ');

  cv.cvtColor(hsv_pal, bgr_pal, cv.COLOR_HSV2BGR);

  palette3 = [...bgr_pal];
  console.log('palette3', dumpPalette(palette3));

  let start = [],
    end = [],
    a = [],
    b = [];
  for(let j = 0; j < 15; j++) {
    start[j] = randLine();

    a[j] = randLine();
    b[j] = randLine();
  }
  console.log('a', a);
  console.log('b', b);

  for(let i = 0; i < images.length; i++) {
    for(let j = 0; j < 15; j++) {
      let coords = [a[j].at(i / (images.length - 1)), b[j].at(i / (images.length - 1))];

      let color = palette3[j];
      cv.line(images[i], ...coords, [...color.slice(0, 3), 255], 3, false);
    }
    for(let j = 0; j < 15; j++) {
      let color = palette3[j];
      let rect = new cv.Rect(new cv.Point(0, j * 20), new cv.Point(100, (j + 1) * 20));
      cv.rectangle(images[i], rect, [...color.slice(0, 3), 255], cv.FILLED, false);
    }

    cv.imwrite(`image-${(i + '').padStart(3, '0')}.png`, images[i]);
  }

  cv.imwrite('output.gif', images, palette3, 10, 15, 0);
}

main(...scriptArgs.slice(1));
