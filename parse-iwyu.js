import fs from "fs";

function main() {
  let data = "\n" + fs.readFileSync("iwyu.txt");
  let i = 0;
  let paragraphs = data.split(/\n\n+/g).filter(p => !/has correct/.test(p));
  let files = {};

  for(let paragraph of paragraphs) {
    let p = paragraph.trim();
    if(p == "") continue;

    p = p.replace("The full include-list for ", "");

    let lines = p
      .replace(/-\s#include/g, "#include")
      .split(/\n/g)
      .filter(line => !/^-+$/.test(line));

    if(!/:$/.test(lines[0])) throw new Error(`Invalid paragraph: ${lines[0]}`);

    let fields = lines.shift().split(/[\s:]+/g);
    let [file, , what='full'] = fields;

    if(!files[file]) files[file] = {};

    console.log(`p#${i}`, console.config({ compact: 0 }), { file, what, lines });

    files[file][what] = lines;

    i++;
  }
  console.log("files", console.config({compact:0}), files);
}

main();
