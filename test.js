const { getDevices, InputStream } = require("./index.js");

console.log('getDevices()', getDevices());

const stream = new InputStream({
  device: 0,
  callback: (err, frame) => {
    //console.log(frame);
  },
});
console.log('stream', stream);
console.log('stream.start()', stream.start());
// console.log(stream.stop());
setTimeout(() => {
  console.log('stream.stop()', stream.stop());
  console.log('stream.start()', stream.start());
  // console.log(stream.stop());
  setTimeout(() => {
    console.log('stream.stop()', stream.stop());
  }, 250);
}, 250);
