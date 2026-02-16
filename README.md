# DDO Test App

## Overview
This is a simple Android test app for sending and receiving DDOs (Device Data Objects) using the DaaS library. The app allows you to start a local agent, map remote nodes, and send test DDO values to other devices on the network. All received DDOs are displayed in the in-app log.  

## Steps to Test

1. **Start the Agent**  
   - Open the app.  
   - Tap the **Start Agent** button.  
   - The app will display your local IP and DIN in the UI and log.  

2. **Configure Remote Node**  
   - Enter the **Remote Node IP** in the first input field.  
   - Enter the **Remote DIN** in the second input field.  

3. **Map the Remote Node**  
   - Tap the **Map Remote Node** button.  
   - The app will map your local agent to the remote DIN.  
   - Log will confirm successful mapping.  

4. **Set a Payload Value**  
   - Enter a signed byte value (e.g., -128 to 127) in the **Payload value** input.  

5. **Send Test DDO**  
   - Tap the **Send Test DDO** button **twice**.  
   - The log will show the DDO sent to the remote node.  

6. **View Logs**  
   - Scroll the **LOG** section at the bottom of the app to see sent and received DDOs.  
   - Incoming DDOs will appear as:  
     ```
     [DDO RECEIVED] from <origin DIN> value=<payload>
     ```  
   - Auto-pulled DDOs will appear as:  
     ```
     [AUTO-PULL] origin=<origin DIN> value=<payload>
     ```  

## Notes
- Make sure both devices (or emulator instances) are on the same network.  
- Only enter valid IPs and DIN values; otherwise, mapping and sending will fail.  
- The payload is a signed byte; values outside -128 to 127 will cause errors.  
