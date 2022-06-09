const express = require("express");
const { Server } = require("http");
const path = require("path");
const SocketIO = require("socket.io");
const { InputStream, getDevices } = require("rtaudio");

const app = express();
const server = new Server(app);
const io = SocketIO(server);

function startAudio() {
  console.log("Audio devices:\n", getDevices());
  console.log("Opening audio stream");
  let gotFrame = false;
  const stream = new InputStream({
    callback: (err, frame) => {
      if (err) {
        console.error("stream callback error:", err);
        console.info('Retrying in 1s');
        setTimeout(startAudio, 1000);
        return;
      }
      if (!gotFrame) {
        console.log("Got first audio frame:", frame);
        gotFrame = true;
      }
      io.emit("audioframe", frame);
    },
  });
  try {
    stream.start();
  } catch (err) {
    console.error('stream.start() error:', err);
    console.info('Retrying in 1s');
    setTimeout(startAudio, 1000);
  }
}

startAudio();

app.get("/", (req, res) => {
  res.sendFile(path.join(__dirname, "static/index.html"));
});
app.use("/static", express.static("static"));

io.on("connection", (socket) => {
  console.log("user connected");
  socket.on("disconnect", () => {
    console.log("user disconnected");
  });
});

const host = "0.0.0.0";
const port = 3000;
server.listen(port, host, () => {
  console.log(`Server is running in http://${host}:${port}/`);
});
