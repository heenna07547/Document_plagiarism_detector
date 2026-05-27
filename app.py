from flask import Flask, render_template, request
import os

app = Flask(__name__)

UPLOAD="uploads"
os.makedirs(UPLOAD,exist_ok=True)

@app.route("/")
def home():
    return render_template("index.html")


@app.route("/analyze",methods=["POST"])
def analyze():

    files=request.files.getlist("files")

    paths=[]

    for f in files:

        p=os.path.join(UPLOAD,f.filename)

        f.save(p)

        paths.append(p)

    cmd="plagiarism_detector "

    cmd+=" ".join(paths)

    os.system(cmd)

    return open("report.html").read()


if __name__=="__main__":
    app.run(debug=True)