from flask import Flask, request, jsonify, send_from_directory
import subprocess
import os

app = Flask(__name__)

UPLOAD="uploads"

os.makedirs(UPLOAD,exist_ok=True)


@app.route("/")
def home():
    return send_from_directory(".", "index.html")


@app.route("/analyze", methods=["POST"])
def analyze():

    file1=request.files["file1"]
    file2=request.files["file2"]

    p1=os.path.join(UPLOAD,file1.filename)
    p2=os.path.join(UPLOAD,file2.filename)

    file1.save(p1)
    file2.save(p2)

    exe_path = r".\plagiarism_detector.exe"
    result=subprocess.run(
        [
            exe_path,
            p1,
            p2
        ],
        capture_output=True,
        text=True
    )

    return jsonify({
        "result": result.stdout
    })


app.run(debug=True)