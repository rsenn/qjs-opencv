import { CV_8UC1, CV_8UC4, Mat, Point, Size, drawLine, imread, imwrite, paletteGenerate, paletteMatch } from 'opencv';

function main(...args) {
  let images = [],
    images2 = [];
  let palette = [],
    palette2 = [];
  let size = new Size(640, 480);

  let paletteImage = imread('lsd/images/building.jpg');

  palette = paletteGenerate(paletteImage, (0 << 1) + 1, 16);

  for(let i = 0; i < 10; i++) {
    let mat = new Mat(size, CV_8UC4);

    images.push(mat);
  }
  for(let i = 0; i < 16; i++) {
    palette2[i] = [i & 1, i & 2, i & 4].map(n => !!n | 0).map(n => n * (i & 8 ? 255 : 128));
  }
  const ansiColor = (r, g, b, bg = false) => `\x1b[${bg ? 48 : 38};2;${r};${g};${b}m`;
  const noColor = () => `\x1b[0m`;

  for(let i = 0; i < palette.length; i++) {
    const [r, g, b] = palette[i];

    console.log(ansiColor(r, g, b, true) + `palette #${i}` + noColor(), palette[i]);
  }

  const randInt = max => Math.floor(Math.random() * max);
  const randCoord = () => new Point(randInt(size.width), randInt(size.height));
  console.log('palette', palette);

  for(let i = 0; i < images.length; i++) {
    for(let j = 0; j < 10; j++) {
      let coords = [randCoord(), randCoord()];
      let color = palette[j];
      /* console.log('coords', coords.map(({ x, y }) => x + ',' + y).join(' -> '));
      console.log('color', color);
      console.log('j', j);*/
      drawLine(images[i], ...coords, [...color.slice(0, 3), 255], 3, false);
    }
    images2[i] = Mat.zeros(size, CV_8UC1);

    paletteMatch(images[i], images2[i], palette, 15);

    //imwrite(`test-${i}.png`, images[i]/*, palette*/);
    imwrite(`test-${i}.png`, images2[i], palette, 15);
  }
  console.log(
    'palette2',
    palette2
      .map(c => c.map(n => (n + '').padStart(3)).join(', '))
      .map(c => `[ ${c} ]`)
      .join('\n'),
  );
}

main(...scriptArgs.slice(1));
