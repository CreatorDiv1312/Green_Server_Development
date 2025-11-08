import streamlit as st
import requests
import pandas as pd
import time
import numpy as np
import base64

def get_audio_base64(file_path):
    with open(file_path, "rb") as f:
        data = f.read()
    return base64.b64encode(data).decode()

alarm_base64 = get_audio_base64("Alarm.wav")

alarm_audio = f"""
<audio controls autoplay loop>
    <source src="data:audio/wav;base64,{alarm_base64}" type="audio/wav">
    Your browser does not support the audio element.
</audio>
"""


st.set_page_config(
    page_title="Jungle Monitoring Dashboard",
    layout="wide",
    page_icon="🌿"
)

# --- Background Video ---
video_html = """
<style>
[data-testid="stAppViewContainer"] {
    background: url('https://cdn.pixabay.com/video/2021/04/10/70125-528698958_large.mp4');
    background-size: cover;
}
</style>
"""
st.markdown(video_html, unsafe_allow_html=True)

# --- Title ---
st.title("🌳 Jungle Sensor Monitoring Dashboard")

API_URL = "http://127.0.0.1:5000/data"

# --- Auto-refresh data ---
st.sidebar.header("🔄 Refresh Settings")
refresh_rate = st.sidebar.slider("Refresh interval (sec)", 5, 60, 10)

placeholder = st.empty()

# --- Alarm sound ---
# alarm_audio = """
# <audio autoplay>
#   <source src="Alarm.wav" type="audio/wav">
# </audio>
# """

# --- Thresholds ---
THRESHOLDS = {
    "temperature": 40,
    "humidity_low": 20,
    "pressure_low": 950,
    "pressure_high": 1050,
    "uv_index": 7,
    "gunfire": 1
}

while True:
    try:
        res = requests.get(API_URL)
        data = res.json().get("data", [])
        df = pd.DataFrame(data)

        if not df.empty:
            # --- Convert Rain Sensor ---
            if "RAIN_Sensing" in df.columns:
                df["Rain Status"] = df["RAIN_Sensing"].apply(lambda x: "🌧️ Raining" if int(x) == 1 else "☀️ No Rain")

            alert_messages = []

            # --- Check thresholds ---
            latest = df.iloc[0]  # Latest data point
            if "temperature" in df.columns and float(latest["temperature"]) > THRESHOLDS["temperature"]:
                alert_messages.append("🔥 Temperature too high")
            if "humidity" in df.columns and float(latest["humidity"]) < THRESHOLDS["humidity_low"]:
                alert_messages.append("💧 Humidity too low")
            if "pressure" in df.columns and (float(latest["pressure"]) < THRESHOLDS["pressure_low"] or float(latest["pressure"]) > THRESHOLDS["pressure_high"]):
                alert_messages.append("⚠️ Pressure out of range")
            if "uv_index" in df.columns and float(latest["uv_index"]) > THRESHOLDS["uv_index"]:
                alert_messages.append("☀️ High UV exposure")
            if "gunfire" in df.columns and int(latest["gunfire"]) == THRESHOLDS["gunfire"]:
                alert_messages.append("💥 Gunfire detected!")

            # --- Apply highlighting to abnormal values ---
            def highlight_alerts(val, col):
                try:
                    if col == "temperature" and float(val) > THRESHOLDS["temperature"]:
                        return "background-color: red; color: white;"
                    elif col == "humidity" and float(val) < THRESHOLDS["humidity_low"]:
                        return "background-color: orange; color: white;"
                    elif col == "pressure" and (float(val) < THRESHOLDS["pressure_low"] or float(val) > THRESHOLDS["pressure_high"]):
                        return "background-color: yellow; color: black;"
                    elif col == "uv_index" and float(val) > THRESHOLDS["uv_index"]:
                        return "background-color: purple; color: white;"
                    elif col == "gunfire" and int(val) == 1:
                        return "background-color: crimson; color: white;"
                except:
                    pass
                return ""

            with placeholder.container():
                # --- Rain Status ---
                if "Rain Status" in df.columns:
                    latest_rain = df["Rain Status"].iloc[0]
                    st.markdown(f"### 🌦️ Current Weather: **{latest_rain}**")

                # --- Show alerts ---
                if alert_messages:
                    st.markdown("## 🚨 ALERTS TRIGGERED:")
                    for msg in alert_messages:
                        st.error(msg)
                    st.markdown(alarm_audio, unsafe_allow_html=True)
                else:
                    st.success("✅ All sensor readings within safe limits.")
                # --- Disable Alarms ----
                if alert_messages:
                    st.markdown(alarm_audio, unsafe_allow_html=True)
                else:
                    st.markdown("")  # stops showing (and thus stops sound)

                # --- Highlight abnormal cells ---
                styled_df = df.style.apply(
                    lambda x: [highlight_alerts(v, x.name) for v in x],
                    axis=0
                )

                st.dataframe(styled_df)

                # --- Charts and Map ---
                if {"temperature", "humidity", "pressure"}.issubset(df.columns):
                    st.line_chart(df[["temperature", "humidity", "pressure"]])

                if {"gps_lat", "gps_lon"}.issubset(df.columns):
                    st.map(df.rename(columns={"gps_lat": "lat", "gps_lon": "lon"}))
        else:
            st.warning("No data found from API.")

    except Exception as e:
        st.error(f"Error fetching data: {e}")

    time.sleep(refresh_rate)
