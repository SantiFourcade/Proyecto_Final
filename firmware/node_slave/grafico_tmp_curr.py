import serial
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from collections import deque

# Puerto serie
ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)

MAX_POINTS = 100

# Buffers
ax_data = deque(maxlen=MAX_POINTS)
ay_data = deque(maxlen=MAX_POINTS)
az_data = deque(maxlen=MAX_POINTS)

temp_data = deque(maxlen=MAX_POINTS)
corr_data = deque(maxlen=MAX_POINTS)
rpm_data = deque(maxlen=MAX_POINTS)

# Figura
fig, (g1, g2, g3, g4) = plt.subplots(4, 1, figsize=(10, 9), sharex=True)

# Líneas
line_ax, = g1.plot([], [], label='Ax')
line_ay, = g1.plot([], [], label='Ay')
line_az, = g1.plot([], [], label='Az')

line_temp, = g2.plot([], [], label='Temperatura (°C)')

line_corr, = g3.plot([], [], label='Corriente')

line_rpm, = g4.plot([], [], label='RPM')

# Títulos
g1.set_title("Aceleración")
g2.set_title("Temperatura")
g3.set_title("Corriente")
g4.set_title("Velocidad")

g1.legend()
g2.legend()
g3.legend()
g4.legend()

g1.grid(True)
g2.grid(True)
g3.grid(True)
g4.grid(True)

def update(frame):
    try:
        line = ser.readline().decode('utf-8').strip()

        if line:
            values = line.split(',')

            if len(values) == 6:
                ax, ay, az, temp, corr, rpm = map(float, values)

                ax_data.append(ax)
                ay_data.append(ay)
                az_data.append(az)

                temp_data.append(temp)
                corr_data.append(corr)
                rpm_data.append(rpm)

                x = range(len(ax_data))

                line_ax.set_data(x, ax_data)
                line_ay.set_data(x, ay_data)
                line_az.set_data(x, az_data)

                line_temp.set_data(x, temp_data)
                line_corr.set_data(x, corr_data)
                line_rpm.set_data(x, rpm_data)

                for ax_plot in (g1, g2, g3, g4):
                    ax_plot.relim()
                    ax_plot.autoscale_view()
                    ax_plot.set_xlim(0, MAX_POINTS)

    except Exception as e:
        print("Error:", e)

    return (
        line_ax, line_ay, line_az,
        line_temp,
        line_corr,
        line_rpm
    )

ani = FuncAnimation(
    fig,
    update,
    interval=50,
    blit=False,
    cache_frame_data=False
)

plt.tight_layout()
plt.show()
