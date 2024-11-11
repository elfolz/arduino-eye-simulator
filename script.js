const bleServiceUUID = '098f6372-826e-43c8-a45f-d29382e382d4'
const bleCharacteristicUUID = '9e423521-9ca6-4ed2-b0f4-079653fdc879'
const decoder = new TextDecoder()
const usbBaudRate = 115200

var usbDevice
var bleDevice
var deviceInfo
var reader
var dialog
var buttonUSB
var buttonBLE
var chunks = ""

if (location.protocol.startsWith('https')) {
	navigator.serviceWorker?.register('./service-worker.js').then(reg => {
		reg.addEventListener('updatefound', () => {
			let newWorker = reg.installing
			newWorker?.addEventListener('statechange', () => {
				console.log('Update Installed. Restarting...')
				if (newWorker.state == 'activated') location.reload(true)
			})
		})
	})
}

function init() {
	dialog = document.querySelector('dialog')
	buttonUSB = document.querySelector('#connectUSB')
	buttonBLE = document.querySelector('#connectBLE')
	if (!'serial' in navigator && !'bluetooth' in navigator) {
		return openDialog('Este App não funciona em neste dispositivo 😥', true)
	}
	if (!'serial' in navigator) buttonUSB.style.setProperty('display', 'none')
	if (!'bluetooth' in navigator) buttonBLE.style.setProperty('display', 'none')
	waitForDevice()
	/* navigator.serial.getPorts()
	.then(response => {
		if (response?.length) connectSerialDevice(response[0])
	}) */
	dialog.querySelector('section svg').onclick = () => {
		closeDialog()
	}
}

function waitForDevice() {
	buttonUSB.onclick = () => {
		navigator.serial.requestPort({
			filters: [{usbVendorId: 0x10C4}]
		})
		.then(device => {
			disconnectSerialDevice()
			if (device) connectSerialDevice(device)
		})
		.catch(e => {
			buttonUSB.removeAttribute('disabled')
			openDialog('Falha ao conectar')
			console.error(e)
		})
	}
	buttonBLE.onclick = () => {
		navigator.bluetooth.requestDevice({
			filters: [{services: [bleServiceUUID]}]
		})
		.then(device => {
			if (device) connectBLEDevice(device)
		})
		.catch(e => {
			buttonBLE.removeAttribute('disabled')
			openDialog('Falha ao conectar')
			console.error(e)
		})
	}
}

function connectSerialDevice(device) {
	usbDevice = device
	device.open({baudRate: usbBaudRate})
	.then(() => {
		deviceInfo = `${device.getInfo().usbVendorId}__${device.getInfo().usbProductId}`
		console.info(`Connected to ${deviceInfo}`)
		disableAll()
		reader = device.readable.getReader()
		read()
	})
	.catch(e => {
		usbDevice?.forget()
		openDialog('Falha ao conectar')
		console.error(e)
	})
}

async function disconnectSerialDevice() {
	try {
		await reader?.cancel()
		await reader?.releaseLock()
		await usbDevice?.close()
	} catch(e){}
	reader = undefined
}

function connectBLEDevice(device) {
	bleDevice = device
	device.gatt.connect(device)
	.then(server => {
		device.addEventListener('gattserverdisconnected', () => {
			openDialog('Bluetooth desconectado')
			buttonBLE.removeAttribute('disabled')
		})
		return server.getPrimaryService(bleServiceUUID)
		.then(service => {
			return service.getCharacteristic(bleCharacteristicUUID)
			.then(characteristic => {
				deviceInfo = device.id
				console.info(`Connected to ${deviceInfo}`)
				disableAll()
				return characteristic.startNotifications()
				.then(() => {
					characteristic.addEventListener('characteristicvaluechanged', e => {
						writeText(e.target.value)
					})
				})
			})
		})
	})
	.catch(e => {
		console.error(e)
		openDialog('Falha ao conectar')
		enableAll()
	})
}

function read() {
	if (!reader) return
	reader.read()
	.then(response => {
		if (/\n/.test(chunks)) {
			writeText(chunks)
			chunks = ""
		}
		chunks += decoder.decode(response.value)
		requestAnimationFrame(read)
	})
	.catch(e => {
		console.error(e)
		openDialog('Falha ao ler dados')
		disconnectSerialDevice()
		enableAll()
	})
}

function writeText(text) {
	if (document.hidden) return
	console.log(text)
	/* document.querySelector('#content').innerHTML += text
	document.querySelector('#caret').scrollIntoView({behavior: 'smooth'}) */
}

function openDialog(text, disableClose=false) {
	dialog.querySelector('section span').innerHTML = text
	if (disableClose) dialog.querySelector('section svg').style.setProperty('display', 'none')
	else dialog.querySelector('section svg').style.removeProperty('display')
	dialog.classList.add('open')
	try { navigator.vibrate(100) } catch(e) {}
}

function closeDialog() {
	dialog.classList.add('close')
	setTimeout(() => {
		dialog.classList.remove('open')
		dialog.classList.remove('close')
	}, 300)
}

function enableAll() {
	buttonBLE.removeAttribute('disabled')
	buttonUSB.removeAttribute('disabled')
}

function disableAll() {
	buttonBLE.setAttribute('disabled', 'true')
	buttonUSB.setAttribute('disabled', 'true')
}

document.onreadystatechange = () => {
	if (document.readyState == 'complete') init()
}
window.onbeforeunload = async () => disconnectSerialDevice()