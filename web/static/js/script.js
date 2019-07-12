function sendToken(token) {
  console.log('Sending token: ' + token);

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
        const t = Uint8Array.from(token, c => c.charCodeAt(0));
        return characteristic.writeValue(t);
      })
      .then(_ => {
        alert('Successfully sent token!');
      })
      .catch(error => {
        alert('Something went wrong:\n' + error);
        console.log('Argh! ' + error);
      });
}

function openUserModal(name, from, to, validity_time) {
  if (arguments.length === 0) {
    $('#input-secret-key').val('');
    $('#input-validity-time').val('');
    $('#input-username').val('');

    $('#input-valid-from-date').val('');
    $('#input-valid-to-date').val('');

    $('#input-valid-from-time').val('');
    $('#input-valid-to-time').val('');
  } else {
    $('#input-secret-key').val('');
    $('#input-validity-time').val(validity_time);
    $('#input-username').val(name);
    const fromDate = new Date(from);
    const toDate = new Date(to);

    document.getElementById('input-valid-from-date').valueAsDate = fromDate;
    document.getElementById('input-valid-to-date').valueAsDate = toDate;

    $('#input-valid-from-time').val(fromDate.getHours() + ':' + fromDate.getMinutes() + ':' + fromDate.getSeconds());
    $('#input-valid-to-time').val(toDate.getHours() + ':' + toDate.getMinutes() + ':' + toDate.getSeconds());
  }

  $('#add-user-modal').modal();
}

function disableUser(name) {

}

function enableUser(name) {

}
