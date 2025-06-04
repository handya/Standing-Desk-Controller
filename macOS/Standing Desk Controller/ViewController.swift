//
//  ViewController.swift
//  Standing Desk Controller
//
//  Created by Andrew Farquharson on 29/01/2025.
//

import Cocoa
import ORSSerial

enum DeskState: String {
    case idle
    case movingUp
    case movingDown
    case manual
    case home
    case unhomed
    case unknown
}

class ViewController: NSViewController {
    @IBOutlet weak var heightSlider: NSSlider!

    var isConnected: Bool = false

    private var serialPort: ORSSerialPort?

    @IBOutlet weak var stateLabel: NSTextField!

    @IBOutlet weak var heightLabel: NSTextField!
    
    @IBOutlet weak var progress: NSProgressIndicator!


    override func viewDidLoad() {
        super.viewDidLoad()
        title = "Standing Desk Controller"
        let ports = ORSSerialPortManager.shared().availablePorts
        print(ports)

        serialPort = ORSSerialPort(path: "/dev/cu.usbserial-840")!
        serialPort?.baudRate = 9600
        serialPort?.delegate = self
        serialPort?.open()
    }


    @IBAction func sliderDidChange(_ sender: Any) {
        guard let slider = sender as? NSSlider else { return }
        let data = "\(Int(slider.doubleValue))\n".data(using: .utf8)!
        serialPort?.send(data)
    }

    @IBAction func buttonOnePressed(_ sender: Any) {
        let data = "a\n".data(using: .utf8)!
        serialPort?.send(data)
    }

    @IBAction func buttonTwoPressed(_ sender: Any) {
        let data = "b\n".data(using: .utf8)!
        serialPort?.send(data)
    }

    @IBAction func buttonThreePressed(_ sender: Any) {
        let data = "c\n".data(using: .utf8)!
        serialPort?.send(data)
    }

    @IBAction func stopButtonPressed(_ sender: Any) {
        let data = "s\n".data(using: .utf8)!
        serialPort?.send(data)
    }
    
    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }
}

extension ViewController: ORSSerialPortDelegate {
    func serialPortWasRemovedFromSystem(_ serialPort: ORSSerialPort) {

    }

    func serialPortWasOpened(_ serialPort: ORSSerialPort) {
        isConnected = true
        print("Serial port \(serialPort.path) opened successfully")
    }

    func serialPortWasClosed(_ serialPort: ORSSerialPort) {
        isConnected = false
        print("Serial port \(serialPort.path) was closed")
    }

    func serialPort(_ serialPort: ORSSerialPort, didReceive data: Data) {
        if let string = String(data: data, encoding: .utf8) {
            for line in string.components(separatedBy: "\n") where line.contains("\r") {
                if let hight = Int(line.trimmingCharacters(in: .whitespacesAndNewlines)) {
                    //print("height: <\(hight)>")
                    heightLabel.stringValue = "\(hight) mm"
                    progress.doubleValue = Double(hight)
                } else if let state = DeskState(rawValue: line.trimmingCharacters(in: .whitespacesAndNewlines)) {
                    //print("state: <\(state)>")
                    stateLabel.stringValue = state.rawValue
                    if state == .unhomed {
                        progress.doubleValue = 0
                        heightLabel.stringValue = "- mm"
                    }
                }
            }
        }
    }

    func serialPort(_ serialPort: ORSSerialPort, didEncounterError error: Error) {
        print("Error on \(serialPort.path): \(error)")
    }
}
