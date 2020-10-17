msfvenom -a x86 --platform Windows -p windows/messagebox TEXT="This is an injection demo!" TITLE="Injection Demo"	-f c
