import React, { Component } from "react";
import SelectVM from "../includes/SelectVM.js"
import { DataContext } from "../includes/Context.js";

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
                
                        <div className="col">
                            <div className="card shadow mt-4" style = {{ backgroundColor: `skyblue` }}>
                                <div className="row card-body">
                                    <div className="scrollable overflow-auto p-3 mb-3 mb-md-0 mr-md-3" style= {{ height: `35rem`, backgroundColor: `skyblue` }}>
                                        <p class = 'card-text'>
                                        Note: don't use .row.justify-content-center instead of .d-flex.justify-content-center, as .row applies negative margins on certain responsiveness intervals, which results into unexpected horizontal scrollbars (unless .row is a direct child of .container, which applies lateral padding to counteract the negative margin, on the correct responsiveness intervals). If you must use .row, for whatever reason, override its margin and padding with .m-0.p-0, in which case you end up with pretty much the same styles as .d-flex.

        Important note: The second solution is problematic when the centered content (the button) exceeds the width of the parent (.d-flex) especially when the parent has viewport width, specifically because it makes it impossible to horizontally scroll to the start of the content (left-most).
        So don't use it when the content to be centered could become wider than the available parent width and all content should be accessible.
        Note: don't use .row.justify-content-center instead of .d-flex.justify-content-center, as .row applies negative margins on certain responsiveness intervals, which results into unexpected horizontal scrollbars (unless .row is a direct child of .container, which applies lateral padding to counteract the negative margin, on the correct responsiveness intervals). If you must use .row, for whatever reason, override its margin and padding with .m-0.p-0, in which case you end up with pretty much the same styles as .d-flex.

        Important note: The second solution is problematic when the centered content (the button) exceeds the width of the parent (.d-flex) especially when the parent has viewport width, specifically because it makes it impossible to horizontally scroll to the start of the content (left-most).
        So don't use it when the content to be centered could become wider than the available parent width and all content should be accessible.
        Note: don't use .row.justify-content-center instead of .d-flex.justify-content-center, as .row applies negative margins on certain responsiveness intervals, which results into unexpected horizontal scrollbars (unless .row is a direct child of .container, which applies lateral padding to counteract the negative margin, on the correct responsiveness intervals). If you must use .row, for whatever reason, override its margin and padding with .m-0.p-0, in which case you end up with pretty much the same styles as .d-flex.

        Important note: The second solution is problematic when the centered content (the button) exceeds the width of the parent (.d-flex) especially when the parent has viewport width, specifically because it makes it impossible to horizontally scroll to the start of the content (left-most).
        So don't use it when the content to be centered could become wider than the available parent width and all content should be accessible.

                                        </p>
                                    </div>
                                </div>
                            </div>
                        </div>
                        
                    </div>
                </div>
            </div>
    );
    }
}

export default Syscalls;