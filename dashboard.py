import streamlit as st
import requests
import pandas as pd
import time

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

while True:
    try:
        res = requests.get(API_URL)
        data = res.json().get("data", [])
        df = pd.DataFrame(data)

        with placeholder.container():
            st.subheader("📊 Latest Sensor Data")
            st.dataframe(df)

            if not df.empty:
                st.line_chart(df[["temperature", "humidity", "pressure"]])
                st.map(df.rename(columns={"gps_lat": "lat", "gps_lon": "lon"}))
    except Exception as e:
        st.error(f"Error fetching data: {e}")

    time.sleep(refresh_rate)
