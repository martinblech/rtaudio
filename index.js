var addon = require("bindings")("rtaudio");

/**
 * Get the host's audio devices.
 */
exports.getDevices = function getDevices() {
  return addon.getDevices();
};

exports.InputStream = class InputStream {
  constructor(options) {
    this._wrapped = new addon.InputStream(options || {});
  }

  start() {
    return this._wrapped.start();
  }

  stop() {
    return this._wrapped.stop();
  }
};
