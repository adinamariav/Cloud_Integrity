import React, { Component } from "react";
import VM from "../includes/VM.js"
import { DataContext } from "../includes/Context.js";
import SyscallTable from "../includes/SyscallTable.js";
import SandboxStatus from "../includes/SandboxStatus.js"
import Status from "../includes/Status.js";
import UploadFile from "../includes/UploadFile.js";
import SyscallInput from "../includes/SyscallInput.js";

class Sandbox extends Component {

    static contextType = DataContext;
    
    render () {
        return(
            <div className="container-fluid ml-5">
                <div className="card" style= {{ maxWidth:`95%` }}>
                    <div className="row card-body" style = {{ backgroundColor: `#D3E0EA`, minHeight:`90vh`}}>
                        <div className="col center" style =  {{ display: 'flex', 
                                                                alignItems: 'center',
                                                                flexDirection: `col`,
                                                                marginTop: `-3rem`,
                                                                justifyContent: 'center'}} >
                            <div className="row">
                                <UploadFile />
                                <VM />
                                <SandboxStatus />
                            </div>
                        </div>
                
                        <div className="col">
                            <Status />
                            <SyscallTable />
                            <SyscallInput />
                        </div>
                        
                    </div>
                </div>
            </div>
    );
    }
}

export default Sandbox;