const { getDevices, InputStream } = require("./index.js");

console.log(getDevices());

const stream = new InputStream({
  device: 0,
  callback: (frame) => {
    console.log(frame);
  },
});
console.log(stream);
console.log(stream.start());
// console.log(stream.stop());
setTimeout(() => {
  console.log(stream.stop());
  console.log(stream.start());
  // console.log(stream.stop());
  setTimeout(() => {
    console.log(stream.stop());
  }, 250);
}, 250);
