from flask import Flask, request, jsonify
import sqlite3
from datetime import datetime

app = Flask(__name__)

# Initialize database (running once)

def init_db():
    conn = sqlite3.connect("database.db")
    cur = conn.cursor()
    cur.execute("""
            CREATE TABLE IF NOT EXISTS sensor_data(
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                temperature REAL,
                humidity REAL,
                pressure REAL,
                altitude REAL,
                soil_moisture REAL,
                uv_index REAL,
                pir_motion REAL,
                ir_detect REAL,
                gunfire INTEGER,
                gps_lat REAL,
                gps_lon REAL,
                timestamp TEXT
        )
    """)

    conn.commit()
    conn.close()

init_db()

# Route : HOME
# 
@app.route('/')
def home():
    return jsonify({"status":"API running successfully", "message":"Welcome to the Forest Monitoring World !!"}) 


@app.route('/upload', methods=['POST'])
def upload_data():
    try:
        data = request.get_json()

        if not data:
            return jsonify({"error":"No JSON data recieved"}), 400
        conn = sqlite3.connect("database.db")
        cur = conn.cursor()

        cur.execute("""
            INSERT INTO sensor_data (
                temperature, humidity, pressure, altitude,
                soil_moisture, uv_index, pir_motion, ir_detect,
                gunfire, gps_lat, gps_lon, timestamp
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, (
            data.get("temperature"),
            data.get("humidity"),
            data.get("pressure"),
            data.get("altitude"),
            data.get("soil_moisture"),
            data.get("uv_index"),
            data.get("pir_motion"),
            data.get("ir_detect"),
            data.get("gunfire"),
            data.get("gps_lat"),
            data.get("gps_lon"),
            datetime.now().isoformat()
        ))

        conn.commit()
        conn.close()

        print(f"[+] Data recieved and stored at {datetime.now()}")
        return jsonify({"status":"success", "message":"Data stored successfully"})

    except Exception as e :
        print("Error :", e)
        return jsonify({"error": str(e)}), 500
    

# data endpoint 
@app.route("/data", methods = ["GET"])
def get_data():
    try:
        conn = sqlite3.connect("database.db")
        conn.row_factory = sqlite3.Row
        cur = conn.cursor()
        cur.execute("SELECT * FROM sensor_Data ORDER BY id DESC LIMIT 50") # Latest 50
        rows = cur.fetchall()
        conn.close()

        # convert to list of dicts 
        data = [dict(row) for row in rows]
        return jsonify({"status":"success", "data":data}), 200
    
    except Exception as e:
        return jsonify({"status":"error", "message": str(e)}), 500

        
# Run Server

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)