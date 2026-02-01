import { Scalar, Rect, Subdiv2D, CV_8UC3, Size, CV_8UC1, blur, getStructuringElement, MORPH_RECT, IMREAD_COLOR, moveWindow, waitKey, destroyWindow, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, bitwise_not, erode, dilate, imread, imshow, cvtColor, COLOR_BGR2GRAY, LINE_AA, FILLED, LINE_8, Mat, Point, drawCircle, fillConvexPoly, drawPolylines, drawLine, cvRound, } from 'opencv';

function rand() {
  return Math.round(Math.random() * Math.pow(2, 32));
}
/*
function help(char** argv)
{
    cout << "\nThis program demonstrates iterative construction of\n"
            "delaunay triangulation and voronoi tessellation.\n"
            "It draws a random set of points in an image and then delaunay triangulates them.\n"
            "Usage: \n";
    cout << argv[0];
    cout << "\n\nThis program builds the triangulation interactively, you may stop this process by\n"
            "hitting any key.\n";
}*/

function draw_subdiv_point(/*Mat&*/ img, /*Point2f */ fp, /*Scalar*/ color) {
  drawCircle(img, fp, 3, color, FILLED, LINE_8, 0);
}

function draw_subdiv(/* Mat&*/ img, /*Subdiv2D&*/ subdiv, /* Scalar*/ delaunay_color) {
  let triangleList = [];

  subdiv.getTriangleList(triangleList);

  let pt = new Array(3);

  for(let i = 0; i < triangleList.length; i++) {
    let t = triangleList[i];

    pt[0] = new Point(cvRound(t[0]), cvRound(t[1]));
    pt[1] = new Point(cvRound(t[2]), cvRound(t[3]));
    pt[2] = new Point(cvRound(t[4]), cvRound(t[5]));

    drawLine(img, pt[0], pt[1], delaunay_color, 1, LINE_AA, 0);
    drawLine(img, pt[1], pt[2], delaunay_color, 1, LINE_AA, 0);
    drawLine(img, pt[2], pt[0], delaunay_color, 1, LINE_AA, 0);
  }
}

function locate_point(/*Mat&*/ img, /*Subdiv2D&*/ subdiv, /*Point2f*/ fp, /*Scalar*/ active_color) {
  let e0 = 0,
    vertex = 0;

  subdiv.locate(
    fp,
    v => (e0 = v),
    v => (vertex = v),
  );

  //console.log('locate_point', { e0, vertex });

  if(e0 > 0) {
    let e = e0;

    do {
      let org = new Point(),
        dst = new Point();

      if(subdiv.edgeOrg(e, org) > 0 && subdiv.edgeDst(e, dst) > 0) {
        drawLine(img, org, dst, active_color, 3, LINE_AA, 0);
      }

      e = subdiv.getEdge(e, Subdiv2D.NEXT_AROUND_LEFT);
    } while(e != e0);
  }

  draw_subdiv_point(img, fp, active_color);
}

function paint_voronoi(img, subdiv) {
  let facets = [],
    centers = [];

  subdiv.getVoronoiFacetList([], facets, centers);

  //console.log('paint_voronoi', {facets,centers})

  let ifacet,
    ifacets = new Array(1);

  for(let i = 0; i < facets.length; i++) {
    ifacet = new Array(facets[i].length);

    for(let j = 0; j < facets[i].length; j++) ifacet[j] = facets[i][j];

    let color = Scalar(rand() & 255, rand() & 255, rand() & 255);

    fillConvexPoly(img, ifacet, color, 8, 0);

    ifacets[0] = ifacet;

    drawPolylines(img, ifacets, true, Scalar(0, 0, 255), 1, LINE_AA, 0);
    drawCircle(img, centers[i], 3, Scalar(0, 0, 255), FILLED, LINE_AA, 0);
  }
}

function main(...args) {
  let active_facet_color = Scalar(0, 0, 255),
    delaunay_color = Scalar(255, 255, 255);

  let rect = new Rect(0, 0, 600, 600);

  let subdiv = new Subdiv2D(rect);
  let img = new Mat(rect.size, CV_8UC3);

  //img = Scalar(0,0,0,0);

  let win = 'Delaunay Demo';
  imshow(win, img);

  for(let i = 0; i < 200; i++) {
    let fp = new Point((rand() % (rect.width - 10)) + 5, (rand() % (rect.height - 10)) + 5);

    locate_point(img, subdiv, fp, active_facet_color);
    imshow(win, img);

    if(waitKey(100) >= 0) break;

    subdiv.insert(fp);

    img.clear();
    draw_subdiv(img, subdiv, delaunay_color);
    imshow(win, img);

    if(waitKey(100) >= 0) break;
  }

  img.clear();
  paint_voronoi(img, subdiv);
  imshow(win, img);

  waitKey(0);

  return 0;
}

main(...scriptArgs.slice(1));
