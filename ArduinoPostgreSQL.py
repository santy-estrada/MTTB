import serial
import serial.tools.list_ports
import psycopg2
import threading

# Define la precisión y la escala que desees para la columna "empuje"
precision = 5  # El número total de dígitos, ajusta según tus necesidades
scale = 2  # El número de dígitos después del punto decimal, ajusta según tus necesidades

##Puertos en uso
ports = serial.tools.list_ports.comports()

serialInst = serial.Serial()

##Lista que tendrá todos los puertos
portsList = []

##Ciclo que muestra todos los puertos disponibles
for onePort in ports:
    portsList.append(str(onePort))
    print(str(onePort))

#Seleccionar puerto (número)
val = input("Select Port: COM")

##Puerto seleccionado
for x in range(0,len(portsList)):
    if portsList[x].startswith("COM" + str(val)):
        portVar = "COM" + str(val)
        print(portVar)

# Configura la conexión serie con Arduino
baud_rate = 9600  # Debe coincidir con la velocidad de comunicación en Arduino
ser = serial.Serial(portVar, baud_rate)

# Solicita al usuario el nombre de la tabla y el nombre de la hélice
table_name = input("Ingresa el nombre de la tabla: ")
propeller_name = input("Ingresa el nombre de la hélice: ")

# Configura la conexión a la base de datos PostgreSQL
db_connection = psycopg2.connect(
    host='urHost',
    user='postgres',
    password='urPWD',
    database='urDataBase'
)

# Crea un cursor para ejecutar consultas
cursor = db_connection.cursor()

# Verifica si la tabla existe
cursor.execute("SELECT EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = %s)", (table_name,))
table_exists = cursor.fetchone()[0]

# Si la tabla no existe, crea la tabla con los campos predefinidos
if not table_exists:
    print(f"La tabla '{table_name}' no existe en la base de datos.")
    table_structure = (
        f"CREATE TABLE {table_name} ("
        "id SERIAL PRIMARY KEY,"
        "Tiempo VARCHAR(255),"
        f"Corriente NUMERIC({precision},{scale}),"
        f"Empuje NUMERIC({precision},{scale}),"
        "Hélice VARCHAR(255)"
        ")"
    )
    cursor.execute(table_structure)
    print(f"La tabla '{table_name}' ha sido creada.")

# Variable para controlar la generación de datos
generating_data = True

# Función para esperar la entrada del usuario y detener la generación de datos
def wait_for_input():
    global generating_data
    input("Presiona Enter para detener la generación de datos...")
    generating_data = False

# Inicia un hilo para esperar la entrada del usuario
input_thread = threading.Thread(target=wait_for_input)
input_thread.start()

while generating_data:
    data = ser.readline().decode().strip()  # Lee los datos de Arduino
    print(data)  # Puedes imprimir los datos para verificar

    # Parsea los datos según sea necesario (dependiendo de cómo envíes los datos desde Arduino)
    values = data.split(",")  # Suponemos que los datos se envían como "valor1,valor2,valor3"

    # Inserta los datos recibidos de Arduino en la tabla
    insert_query = f"INSERT INTO {table_name} (Empuje, Corriente, Tiempo, Hélice) VALUES (%s, %s, %s, %s)"
    cursor.execute(insert_query, (values[0], values[1], values[2], propeller_name))

    # Confirma la transacción
    db_connection.commit()

# Espera a que el hilo de entrada del usuario termine antes de cerrar las conexiones
input_thread.join()

# Cierra la conexión y el puerto serie cuando termine
cursor.close()
db_connection.close()
ser.close()
