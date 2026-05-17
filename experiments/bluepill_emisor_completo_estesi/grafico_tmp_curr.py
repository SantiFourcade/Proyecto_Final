import serial
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import collections

# Configuración del puerto (ajustá /dev/ttyUSB0 si cambió de número)
ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)

# Almacenamos los 2 tipos de datos que estás mandando
temp_data = collections.deque(maxlen=100)
rms_data  = collections.deque(maxlen=100)

fig, (ax1, ax2) = plt.subplots(2, 1, sharex=True, figsize=(8, 6))
line1, = ax1.plot([], [], 'r-', label='Temp (°C)')
line2, = ax2.plot([], [], 'b-', label='Corriente (V)')

def init():
    ax1.set_ylim(15, 35)  # Rango cómodo para temperatura ambiente
    ax2.set_ylim(-0.1, 0.6) # Rango completo del ADC
    ax1.legend(loc='upper right')
    ax2.legend(loc='upper right')
    return line1, line2

def update(frame):
    try:
        line = ser.readline().decode('utf-8').strip()
        if line:
            # Desglosamos los 2 valores que pusiste en el log
            t, rms = map(float, line.split(','))
            
            temp_data.append(t)
            rms_data.append(rms)
            
            line1.set_data(range(len(temp_data)), temp_data)
            line2.set_data(range(len(rms_data)), rms_data)
            
            ax1.set_xlim(0, 100)
            ax2.set_xlim(0, 100)
    except Exception as e:
        # Descomenta esto si querés ver si falla alguna línea por interferencia
        # print(f"Error parseando línea: {e}")
        pass
    return line1, line2

ani = FuncAnimation(fig, update, init_func=init, blit=True, interval=1)
plt.tight_layout()
plt.show()
