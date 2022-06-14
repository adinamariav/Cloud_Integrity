import React, { Component } from "react";
import SelectVM from "../includes/VM.js"
import { DataContext } from "../includes/Context.js";
import SyscallTable from "../includes/SyscallTable.js";

class Syscalls extends Component {

    static contextType = DataContext;

    render () {
        return(
            <div className="container">
                <div className="card mt-4">
                    <div className="row card-body" style = {{ backgroundColor: `lightblue`}}>

                        <div className="col center" style =  {{ display: 'flex', 
                                                                alignItems: 'center',
                                                                flexDirection: `col`,
                                                                justifyContent: 'center'}} >
                            <div className="row mx-auto">
                                <SelectVM />
                                <button type="button" onClick={() => this.context.start()} class="btn row btn-danger shadow" style= {{ width: `5rem`, marginLeft: `7rem`, marginTop: `5rem`}}>Run</button>
                            </div>
                        </div>
                            <SyscallTable />
                        <div className="col">
                            
                        </div>
                        
                    </div>
                </div>
            </div>
    );
    }
}

export default Syscalls;