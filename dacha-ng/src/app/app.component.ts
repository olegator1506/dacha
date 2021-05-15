import { Component } from '@angular/core';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})
export class AppComponent {
  title = 'dacha-ng';
  channels = [
    {
      name:"Пол",
      tempSensorNum:1,
      controlNum:1
    },
    {
      name:"Обогреватель",
      tempSensorNum:2,
      controlNum:2
    }
  ];
}
