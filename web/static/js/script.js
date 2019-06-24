function sendToken(token) {
  const serviceUUID = '12345678-1234-5678-1234-56789abcdef0';
  const characteristicUUID = '12345678-1234-5678-1234-56789abcdef5';
  const options = {
    filters: [{
      name: 'X1'
    }],
    optionalServices: [
      serviceUUID
    ]
  };

  navigator.bluetooth.requestDevice(options)
    .then(device => {
      console.log('> Name:             ' + device.name);
      console.log('> Id:               ' + device.id);
      console.log('> Connected:        ' + device.gatt.connected);
      return device.gatt.connect();
    })
    .then(server => server.getPrimaryService(serviceUUID))
    .then(service => service.getCharacteristic(characteristicUUID))
    .then(characteristic => {
      const t = Uint8Array.from(atob(token), c => c.charCodeAt(0));
      return characteristic.writeValue(t);
    })
    .then(_ => {
      console.log('Successfully sent token:', token);
    })
    .catch(error => {
      console.log('Argh! ' + error);
    });
}
