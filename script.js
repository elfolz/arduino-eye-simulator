const bleServiceUUID = '0888fddd-b50d-4710-aa9b-17051f1b4948'
const bleCharacteristicReadUUID = 'b2d07323-8815-4a88-9bae-27c835524aee'
const bleCharacteristicWriteUUID = 'd359723f-1a25-477b-9070-e7483e231aef'
const decoder = new TextDecoder()
const encoder = new TextEncoder()
const usbBaudRate = 115200

const eyeColor = localStorage.getItem('eyeColor')

var colorPicker
var usbDevice
var bleDevice
var bleCharacteristicRead
var bleCharacteristicWrite
var bleDescriptor
var deviceInfo
var usbReader
var usbWriter
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
	colorPicker = document.querySelector('#color-picker')
	buttonUSB = document.querySelector('#connect-usb')
	buttonBLE = document.querySelector('#connect-ble')
	if (!'serial' in navigator && !'bluetooth' in navigator) {
		return openDialog('Este App não funciona em neste dispositivo 😥', true)
	}
	if (!'serial' in navigator) buttonUSB.style.setProperty('display', 'none')
	if (!'bluetooth' in navigator) buttonBLE.style.setProperty('display', 'none')
	handleColorPicker()
	waitForDevice()
	dialog.querySelector('section svg').onclick = () => {
		closeDialog()
	}
}

function handleColorPicker() {
	if (localStorage.getItem('eyeColor')) {
		document.querySelector('input[type=color]').value = localStorage.getItem('eyeColor')
	}
	colorPicker.oninput = e => {
		const color = e.target.value
		if (!color) return localStorage.removeItem('eyeColor')
		localStorage.setItem('eyeColor', color)
		syncColor(color)
	}
}

function syncColor(color) {
	const c = `eyeColor${rgb565color(color)}\n`
	if (usbDevice?.connected && usbWriter) {
		usbWriter.write(encoder.encode(c))
	}
	if (bleCharacteristicWrite) {
		try { bleCharacteristicWrite.writeValue(encoder.encode(c)) } catch(e) {}
	}
}

function waitForDevice() {
	buttonUSB.onclick = () => {
		navigator.serial.requestPort({
			filters: [{usbVendorId: 0x10C4}]
		})
		.then(device => {
			console.log(device)
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
		usbReader = device.readable.getReader()
		usbWriter = device.writable.getWriter()
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
		await usbReader?.cancel()
		await usbReader?.releaseLock()
		await usbDevice?.close()
	} catch(e){}
	usbReader = undefined
}

function connectBLEDevice(device) {
	bleDevice = device
	bleDevice.addEventListener('gattserverdisconnected', () => {
		openDialog('Bluetooth desconectado')
		buttonBLE.removeAttribute('disabled')
	})
	device.gatt.connect()
	.then(server => server.getPrimaryService(bleServiceUUID))
	.then(service => service.getCharacteristics())
	.then(characteristics => {
		bleCharacteristicRead = characteristics.find(el => el.uuid == bleCharacteristicReadUUID)
		bleCharacteristicWrite = characteristics.find(el => el.uuid == bleCharacteristicWriteUUID)
		deviceInfo = `${device.name.replace(' ', '-')}__${device.id}`
		console.info(`Connected to ${deviceInfo}`)
		disableAll()
		if (eyeColor) syncColor(eyeColor)
		return bleCharacteristicRead.startNotifications()
		.then(() => {
			bleCharacteristicRead.addEventListener('characteristicvaluechanged', e => {
				writeText(decoder.decode(e.target.value))
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
	usbReader?.read()
	.then(response => {
		if (response.done) {
			return usbReader.releaseLock()
		} else if (/\n/.test(chunks)) {
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
	/* document.querySelector('#content').innerHTML += text
	document.querySelector('#caret').scrollIntoView({behavior: 'smooth'}) */
}

function rgb565color(color) {
	const hex = '0x'+color.replace('#', '')
	const red =  (hex >> 16) & 0xFF
	const green = (hex >> 8) & 0xFF
	const blue = hex & 0xFF
	let red5 = (red >> 3) & 0x1F
	let green6 = (green >> 2) & 0x3F
	let blue5 = (blue >> 3) & 0x1F
	return '0x' + ((red5 << 11) | (green6 << 5) | blue5).toString(16).toUpperCase().padStart(4, '0')
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
	colorPicker.setAttribute('disabled', 'true')
}

function disableAll() {
	buttonBLE.setAttribute('disabled', 'true')
	buttonUSB.setAttribute('disabled', 'true')
	colorPicker.removeAttribute('disabled')
}

document.onreadystatechange = () => {
	if (document.readyState == 'complete') init()
}
/* window.onbeforeunload = async () => disconnectSerialDevice() */