function sendToken(token) {
  const options = {filters: [{services: 'space_lock'}]};

  navigator.bluetooth.requestDevice(options)
      .then(device => {
        console.log('> Name:             ' + device.name);
        console.log('> Id:               ' + device.id);
        console.log('> Connected:        ' + device.gatt.connected);
        return device.gatt.connect();
      })
      .then(server => server.getPrimaryService('space_lock'))
      .then(service => service.getCharacteristic('space_lock_token'))
      .then(characteristic => {
        // Writing 1 is the signal to reset energy expended.
        // const token = Uint8Array.of(1);
        return characteristic.writeValue(token);
      })
      .then(_ => {
        console.log('Energy expended has been reset.');
      })
      .catch(error => {
        console.log('Argh! ' + error);
      });
}
