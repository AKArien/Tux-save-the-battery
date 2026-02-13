# Tux save the battery

![Keep calm and stay on](./keep-calm-and-stay-on.svg)

Please note this software is still in very early development and currently working towards a proof of concept stage. Don’t try to use it yet if you haven’t read and understood all the code and it’s shortcomings. A lot is yet unimplemented, any contributions are of course welcome

TSTB for short is a utility that aims to save battery on linux devices while retaining some functionnality.
Specifically, the goal is to be able to recieve and display notifications, as well as allowing certain programs to run, such as playing music, like common mobile operating systems allow.

To achieve this, we simply forego suspending the system (S3 sleeping state). Instead, we try to save as much battery as we can from S0.
The most basic element of this power save is stopping procceses that don’t need to run and might eat up battery. By using an extremely conservative scheduler, we can also let some programs run the slighest bit they need without impacting the battery use too much. In the end, we will hopefully also be able to integrate some platform/device-specific power saving mechanisms.

Alongside with mantaining functionnality of certain programs, this also has the upside of granting instantaneous wake-ups, and enabling double-taps on touchscreens to wake.

TSTB uses a daemon that monitors processes and their state. When called to by a client, it will apply it’s rules for power saving. It gets them from runtime client configuration, as well as it’s configuration file.
Though it is meant to be interacted with by compositors which should link (dynamically or statically) against the library, a simple tstbc binary is provided for the purposes of scripting or manual use if needed.

# Credit

Many thanks to the creators of assets used for the « keep calm and stay on » art :
- [Keep Calm and Carry On](https://upload.wikimedia.org/wikipedia/commons/3/30/Keep_Calm_and_Carry_On_Poster.svg) (Original: UK Government Vector:  Mononomic, Public domain, via Wikimedia Commons)
- [Tux (vectorial)](https://upload.wikimedia.org/wikipedia/commons/3/35/Tux.svg) (lewing@isc.tamu.edu Larry Ewing and The GIMP, CC0, via Wikimedia Commons)
- [AA battery](https://freesvg.org/aa-battery) (OpenClipart on freesvg.org)
