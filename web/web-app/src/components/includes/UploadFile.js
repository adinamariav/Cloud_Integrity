import axios from 'axios'
import React, { Component } from 'react';
import { DataContext } from './Context';

class UploadFile extends Component {
    
    static contextType = DataContext;

    state = {
        selectedFile: null
    }

    onFileChange = event => {
        this.setState({ selectedFile: event.target.files[0] });
    };

    onFileUpload = () => {
        const {vmName} = this.context;
        const formData = new FormData();

        formData.append('file', this.state.selectedFile);
        formData.append('name', this.state.selectedFile.name);
        formData.append('vmName', vmName);
      
        console.log(this.state.selectedFile.name);

        const config = {
            headers: {
              'content-type': 'multipart/form-data',
            },
        };

        axios.post("/uploadfile", formData, config)
        .then((response) => {
            if (response["data"] === "Success")
                alert("Upload successful!")
            else
                alert("The request failed, please try again!")
        })
    };

      render() {

        const isStarted = this.context.isVMStarted;
        
        if (isStarted) {
            return (
                <div class="card ml-4 shadow" style={{maxWidth:`90%`}}>
                    <div class="card-body container-fluid row">
                        <input class="form-control col" type="file" id="formFile" onChange={this.onFileChange} style={{maxWidth:`20rem`}}/>
                        <button class = "btn col" onClick={this.onFileUpload} style= {{ marginLeft: `18rem`, maxHeight:`80%`, maxWidth:`8rem`, marginTop:`-0.2rem`, backgroundColor:`#276678`, color:`white`}}>
                          Upload
                        </button>
                    </div>
                </div>
              );
        }
      }
}

export default UploadFile