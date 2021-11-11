import * as cv from 'opencv';

function main(...args) {
  let images = [];
  let palette = [];
  let size = new cv.Size(640, 480);

  let paletteImage = cv.imread('lsd/images/building.jpg');

  palette = cv.paletteGenerate(paletteImage, (0 << 1) + 1, 16);

  for(let i = 0; i < 10; i++) {
    let mat = new cv.Mat(size, cv.CV_8UC4);

    images.push(mat);
  }
  const ansiColor = (r, g, b, bg = false) => `\x1b[${bg ? 48 : 38};2;${r};${g};${b}m`;
  const noColor = () => `\x1b[0m`;

  console.log(images);
  for(let i = 0; i < palette.length; i++) {
    const [r, g, b] = palette[i];

    console.log(ansiColor(r, g, b, true) + `palette #${i}` + noColor(), palette[i]);
  }

  const randInt = max => Math.floor(Math.random() * max);
  const randCoord = () => new cv.Point(randInt(size.width), randInt(size.height));
  console.log('palette', palette);

  for(let i = 0; i < images.length; i++) {
    for(let j = 0; j < 10; j++) {
      let coords = [randCoord(), randCoord()];
      let color = palette[j];
      console.log('coords', coords.map(({ x, y }) => x + ',' + y).join(' -> '));
      console.log('color', color);
      console.log('j', j);
      cv.line(images[i], ...coords, [...color.slice(0, 3), 255], 3, false);
    }

    cv.imwrite(`test-${i}.png`, images[i], palette);
  }
}

main(...scriptArgs.slice(1));
