import React, { Component } from "react";
import VM from "../includes/VM.js"
import { DataContext } from "../includes/Context.js";
import SyscallTable from "../includes/SyscallTable.js";
import LearnStatus from "../includes/LearnStatus.js"
import AnalyzeStatus from "../includes/AnalyzeStatus.js";
import Status from "../includes/Status.js";

class Analyze extends Component  {
    static contextType = DataContext;

    render () {
        return(
            <div className="container-fluid mt-4 ml-5">
                <div className="card" style= {{ maxWidth:`95%` }}>
                    <div className="row card-body" style = {{ backgroundColor: `#D3E0EA`, minHeight:`90vh`}}>
                        <div className="col center" style =  {{ display: 'flex', 
                                                                alignItems: 'center',
                                                                flexDirection: `col`,
                                                                justifyContent: 'center'}} >
                            <div className="row">
                                <VM />
                                <LearnStatus />
                                <AnalyzeStatus />
                            </div>
                        </div>
                
                        <div className="col">
                        <Status />
                        <SyscallTable />
                           
                        </div>
                        
                    </div>
                </div>
            </div>
    );
    }
}

export default Analyze;