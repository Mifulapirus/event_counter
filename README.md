# Event Counter

**Event Counter** is a small physical device that lets you log real-world actions with a button press. No apps, no accounts, no dashboards to babysit. Just press a button and your event is recorded in **your own Google Sheet**.

It is designed to be fast, tactile, and distraction-free.

---

## What it does

Event Counter is powered by a **WEMOS D1 Mini** and has two physical buttons labeled **1** and **2**.

When you press a button, the device:
- Logs an entry to your Google Sheet
- Records the action label assigned to that button
- Adds a timestamp
- Stores the remaining battery level

One press, one row.

---

## How it works

1. The device connects to your WiFi  
2. It hosts a small configuration website directly from the microcontroller  
3. You define what each button means (for example: “Drank water”, “Took medicine”, “Started homework”, "Finished homework")  
4. On every button press, the device sends an HTTP request to a Google Apps Script Web App  
5. The script appends the event to your spreadsheet  

There is:
- No mobile app  
- No external backend  
- No third-party cloud services  

Everything lives in **your Google Drive**.

---

## Google Sheets integration

The `google_script/event_counter.gs` file contains the Google Apps Script used to receive events and write them into a spreadsheet.

Each logged event includes:
- Event ID  
- Timestamp  
- Device ID  
- Battery level  
- Action label  
- Optional value  

The script is published as a Web App and accepts simple HTTP GET requests from the device.

See the header comments in `event_counter.gs` for step-by-step setup instructions.

---

## Repository structure

'''
event_counter/
├── event_counter-fw/ # Firmware for the WEMOS D1 Mini
├── event_counter-hw/ # Hardware design files
├── google_script/
│ └── event_counter.gs # Google Apps Script Web App
├── pictures/ # Device photos
└── README.md
'''

---

## Why not just use an app?

This is your very own device, apps belong to someone else and add friction.

Event Counter is built for moments when pulling out a phone feels heavier than the action you are tracking. It favors:
- Physical interaction over screens  
- Simplicity over features  
- Ownership over platforms  

It works well for habits, routines, and any event that should be logged quietly and instantly.

---

## Privacy

- No data is sent anywhere except your Google Sheet  
- No analytics  
- No accounts  
- No vendor cloud  

---

## Status

This project is functional but intentionally simple. It is meant as:
- A practical tool  
- A hardware + software experiment  
- A template for other physical logging devices  

