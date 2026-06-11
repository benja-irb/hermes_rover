import pygame
import time
import serial
import sys

# CAMBIA ESTO AL PUERTO DE TU PICO (ej. COM3, COM5, /dev/ttyACM0)
PUERTO_PICO = "/dev/micro"
BAUDRATE = 115200

def aplicar_zona_muerta(valor, limite=0.08):
    """Filtra el ruido del centro del joystick."""
    return 0.0 if abs(valor) < limite else valor

def main():
    pygame.init()
    pygame.joystick.init()

    if pygame.joystick.get_count() == 0:
        print("No se encontró ningún control conectado.")
        return

    control = pygame.joystick.Joystick(0)
    control.init()
    
    print(f"Control conectado: {control.get_name()}")
    gatillo_activo = None 

    # --- BUCLE MAESTRO DE CONEXIÓN ---
    while True:
        try:
            # Intentamos establecer la conexión (solo se hace una vez hasta que se rompa)
            ser = serial.Serial(PUERTO_PICO, BAUDRATE, timeout=0.1)
            print(f"\n✅ ¡Conexión serial EXITOSA con la Pico en {PUERTO_PICO}!")
            print("Enviando comandos... (Presiona Ctrl+C para salir)\n")
            
            # --- BUCLE DE ENVÍO DE COMANDOS (Alta velocidad) ---
            while True:
                pygame.event.pump()

                lt = (control.get_axis(2) + 1.0) / 2.0
                rt = (control.get_axis(5) + 1.0) / 2.0

                if lt < 0.05: lt = 0.0
                if rt < 0.05: rt = 0.0

                if rt > 0.0 and lt == 0.0:
                    gatillo_activo = 'RT'
                elif lt > 0.0 and rt == 0.0:
                    gatillo_activo = 'LT'
                elif rt == 0.0 and lt == 0.0:
                    gatillo_activo = None 

                if gatillo_activo == 'RT':
                    lt = 0.0
                elif gatillo_activo == 'LT':
                    rt = 0.0

                lx = aplicar_zona_muerta(control.get_axis(0))
                btn_a = control.get_button(0)
                btn_lb = control.get_button(4)
                btn_rb = control.get_button(5)

                trama_serial = f"{lt:.2f},{rt:.2f},{lx:+.2f},{btn_a},{btn_lb},{btn_rb}\n"

                try:
                    # Intentamos enviar el comando
                    ser.write(trama_serial.encode('utf-8'))
                    sys.stdout.write(f"\rEnviando: {trama_serial.strip()}      ")
                    sys.stdout.flush()
                except serial.SerialException:
                    # Si falla al escribir, se desconectó el cable
                    print("\n\n❌ Se perdió la conexión con la Pico. Intentando reconectar...")
                    ser.close()
                    break # Rompe este bucle y vuelve al bucle maestro

                time.sleep(0.05)

        except serial.SerialException:
            # Si no puede abrir el puerto, avisa y espera antes de reintentar
            sys.stdout.write(f"\rBuscando a la Pico en {PUERTO_PICO}... (Asegúrate de cerrar Thonny)   ")
            sys.stdout.flush()
            time.sleep(2.0)
            
        except KeyboardInterrupt:
            # Salida limpia si el usuario presiona Ctrl+C
            print("\n\nLectura finalizada por el usuario.")
            if 'ser' in locals() and ser.is_open:
                ser.close()
            pygame.quit()
            return

if __name__ == "__main__":
    main()